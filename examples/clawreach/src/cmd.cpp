/**
 * ClawReach Serial Console Commands
 * 
 * Commands:
 *   wifi_sta -s <ssid> -p <password>    Set WiFi credentials
 *   clawreach_server -u <url> [-t <token>]  Set server URL and optional token
 *   reboot                               Restart device
 */

#include "main.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

static const char* TAG = "cmd";

#define PROMPT_STR "ClawReach"

// Storage helpers
static esp_err_t storage_write(const char* ns, const char* key, 
                                const void* data, size_t len) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_blob(handle, key, data, len);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

static esp_err_t storage_read(const char* ns, const char* key,
                               void* data, size_t* len) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(ns, NVS_READONLY, &handle);
    if (err != ESP_OK) return err;

    err = nvs_get_blob(handle, key, data, len);
    nvs_close(handle);
    return err;
}

// WiFi config command
static struct {
    struct arg_str* ssid;
    struct arg_str* password;
    struct arg_end* end;
} wifi_args;

static int wifi_cmd(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**)&wifi_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, wifi_args.end, argv[0]);
        return 1;
    }

    if (!wifi_args.ssid->count) {
        ESP_LOGE(TAG, "SSID required");
        return -1;
    }

    wifi_config_t wifi_config = {};
    strlcpy((char*)wifi_config.sta.ssid, wifi_args.ssid->sval[0],
            sizeof(wifi_config.sta.ssid));

    if (wifi_args.password->count) {
        strlcpy((char*)wifi_config.sta.password, wifi_args.password->sval[0],
                sizeof(wifi_config.sta.password));
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }

    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    esp_wifi_stop();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi configured: SSID=%s", wifi_config.sta.ssid);
    return 0;
}

static void register_wifi_cmd(void) {
    wifi_args.ssid = arg_str0("s", NULL, "<ssid>", "WiFi SSID");
    wifi_args.password = arg_str0("p", NULL, "<password>", "WiFi password");
    wifi_args.end = arg_end(2);

    const esp_console_cmd_t cmd = {
        .command = "wifi_sta",
        .help = "Configure WiFi STA mode",
        .hint = NULL,
        .func = &wifi_cmd,
        .argtable = &wifi_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

// ClawReach server config command
static struct {
    struct arg_str* url;
    struct arg_str* token;
    struct arg_end* end;
} server_args;

static int server_cmd(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**)&server_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, server_args.end, argv[0]);
        return 1;
    }

    char url[MAX_SERVER_URL_LEN] = {0};
    char token[MAX_SERVER_TOKEN_LEN] = {0};

    if (server_args.url->count) {
        strlcpy(url, server_args.url->sval[0], sizeof(url));
    } else {
        // Just show current config
        size_t len = sizeof(url);
        if (storage_read(CLAWREACH_NVS_NAMESPACE, CLAWREACH_SERVER_URL_KEY,
                         url, &len) == ESP_OK) {
            ESP_LOGI(TAG, "Current server URL: %s", url);
        } else {
            ESP_LOGI(TAG, "No server URL configured");
        }
        return 0;
    }

    if (server_args.token->count) {
        strlcpy(token, server_args.token->sval[0], sizeof(token));
    }

    // Save config
    esp_err_t err = clawreach_save_config(url, token);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Server config saved: %s", url);
        if (strlen(token) > 0) {
            ESP_LOGI(TAG, "Token: (set)");
        }
    } else {
        ESP_LOGE(TAG, "Failed to save config: %s", esp_err_to_name(err));
        return -1;
    }

    return 0;
}

static void register_server_cmd(void) {
    server_args.url = arg_str0("u", NULL, "<url>", "Server WebSocket URL");
    server_args.token = arg_str0("t", NULL, "<token>", "Auth token (optional)");
    server_args.end = arg_end(2);

    const esp_console_cmd_t cmd = {
        .command = "clawreach_server",
        .help = "Configure ClawReach server",
        .hint = NULL,
        .func = &server_cmd,
        .argtable = &server_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

// Reboot command
static int reboot_cmd(int argc, char** argv) {
    ESP_LOGI(TAG, "Rebooting...");
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_restart();
    return 0;
}

static void register_reboot_cmd(void) {
    const esp_console_cmd_t cmd = {
        .command = "reboot",
        .help = "Reboot the device",
        .hint = NULL,
        .func = &reboot_cmd,
        .argtable = NULL
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

// Config load/save functions
int clawreach_load_config(char* url_out, char* token_out) {
    size_t len;
    esp_err_t err;

    if (url_out) {
        len = MAX_SERVER_URL_LEN;
        err = storage_read(CLAWREACH_NVS_NAMESPACE, CLAWREACH_SERVER_URL_KEY,
                           url_out, &len);
        if (err != ESP_OK) {
            url_out[0] = '\0';
            return 0;
        }
    }

    if (token_out) {
        len = MAX_SERVER_TOKEN_LEN;
        err = storage_read(CLAWREACH_NVS_NAMESPACE, CLAWREACH_SERVER_TOKEN_KEY,
                           token_out, &len);
        if (err != ESP_OK) {
            token_out[0] = '\0';
        }
    }

    return 1;
}

int clawreach_save_config(const char* url, const char* token) {
    esp_err_t err;

    err = storage_write(CLAWREACH_NVS_NAMESPACE, CLAWREACH_SERVER_URL_KEY,
                        url, strlen(url) + 1);
    if (err != ESP_OK) return err;

    if (token && strlen(token) > 0) {
        err = storage_write(CLAWREACH_NVS_NAMESPACE, CLAWREACH_SERVER_TOKEN_KEY,
                            token, strlen(token) + 1);
    }

    return err;
}

// Console initialization
int cmd_init(void) {
    esp_console_repl_t* repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = PROMPT_STR ">";
    repl_config.max_cmdline_length = 512;

    // Register commands
    register_wifi_cmd();
    register_server_cmd();
    register_reboot_cmd();

    // Initialize UART console
#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl));
#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    esp_console_dev_usb_serial_jtag_config_t hw_config = 
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
#endif

    ESP_ERROR_CHECK(esp_console_start_repl(repl));

    ESP_LOGI(TAG, "Console ready. Commands: wifi_sta, clawreach_server, reboot");
    return 0;
}
