/**
 * ClawReach - Physical AI Presence for OpenClaw
 * 
 * Streams audio + video to a self-hosted server running
 * Whisper (speech-to-text) + Vision models.
 * Server returns TTS audio + display commands.
 */

#include "main.h"

#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>

static char g_server_url[MAX_SERVER_URL_LEN] = {0};
static char g_server_token[MAX_SERVER_TOKEN_LEN] = {0};

extern "C" void board_init(void) {
    bsp_io_expander_init();
    lv_disp_t* lvgl_disp = bsp_lvgl_init();
    assert(lvgl_disp != NULL);
    bsp_rgb_init();
    bsp_codec_init();
    bsp_codec_volume_set(100, NULL);
}

extern "C" void long_press_event_cb(void) {
    ESP_LOGI(LOG_TAG, "Long press - shutting down");
    bsp_system_shutdown();
    bsp_lcd_brightness_set(0);
    bsp_codec_mute_set(true);
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_restart();
}

extern "C" void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize hardware
    board_init();
    bsp_set_btn_long_press_cb(long_press_event_cb);

    // Initialize UI
    ui_init();

    // Initialize serial console (for WiFi and server config)
    cmd_init();

    // Try to load saved config
    int config_loaded = clawreach_load_config(g_server_url, g_server_token);
    
    if (!config_loaded || strlen(g_server_url) == 0) {
        ESP_LOGI(LOG_TAG, "No server config found, starting QR scan...");
        // Use WiFi connecting screen as placeholder
        ui_wifi_connecting();
        
        // Try QR code scanning for config
        if (clawreach_qr_scan_config() != 0) {
            ESP_LOGE(LOG_TAG, "No config via QR. Use serial commands:");
            ESP_LOGE(LOG_TAG, "  clawreach_server -u wss://your-server/clawreach");
            ESP_LOGE(LOG_TAG, "  wifi_sta -s SSID -p password");
            
            // Wait for serial config
            while (strlen(g_server_url) == 0) {
                vTaskDelay(pdMS_TO_TICKS(1000));
                clawreach_load_config(g_server_url, g_server_token);
            }
        }
    }

    ESP_LOGI(LOG_TAG, "Server URL: %s", g_server_url);

    // Initialize WiFi
    oai_wifi_init();
    oai_wifi();

    // Initialize camera
    clawreach_camera_init();

    // Initialize audio
    clawreach_init_audio_capture();
    clawreach_init_audio_encoder();
    clawreach_init_audio_decoder();

    // Initialize and connect WebSocket
    clawreach_websocket_init();
    
    ui_listening();
    ESP_LOGI(LOG_TAG, "ClawReach ready!");

    // Main loop
    clawreach_websocket_connect();
    
    while (1) {
        clawreach_websocket_loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
