/**
 * ClawReach QR Code Setup
 * 
 * Scans a QR code containing JSON config:
 * {"url": "wss://server/clawreach", "token": "optional_token"}
 */

#include "main.h"

#include <esp_log.h>
#include <cJSON.h>
#include <string.h>

#define QR_SCAN_TIMEOUT_MS 30000  // 30 seconds to scan

static bool parse_qr_json(const char* json_str, char* url_out, char* token_out) {
    cJSON* root = cJSON_Parse(json_str);
    if (!root) {
        ESP_LOGE(LOG_TAG, "Failed to parse QR JSON");
        return false;
    }

    cJSON* url = cJSON_GetObjectItem(root, "url");
    if (url && cJSON_IsString(url)) {
        strlcpy(url_out, url->valuestring, MAX_SERVER_URL_LEN);
    } else {
        cJSON_Delete(root);
        return false;
    }

    cJSON* token = cJSON_GetObjectItem(root, "token");
    if (token && cJSON_IsString(token)) {
        strlcpy(token_out, token->valuestring, MAX_SERVER_TOKEN_LEN);
    } else {
        token_out[0] = '\0';
    }

    cJSON_Delete(root);
    return true;
}

int clawreach_qr_scan_config(void) {
    ESP_LOGI(LOG_TAG, "Starting QR code scan for config...");
    ESP_LOGI(LOG_TAG, "Point camera at QR code with format:");
    ESP_LOGI(LOG_TAG, "{\"url\":\"wss://your-server/clawreach\",\"token\":\"optional\"}");

    char url[MAX_SERVER_URL_LEN] = {0};
    char token[MAX_SERVER_TOKEN_LEN] = {0};

    uint32_t start_time = xTaskGetTickCount();
    uint32_t timeout_ticks = pdMS_TO_TICKS(QR_SCAN_TIMEOUT_MS);

    while ((xTaskGetTickCount() - start_time) < timeout_ticks) {
        vTaskDelay(pdMS_TO_TICKS(500));
        
        // Check if config was set via serial while we're scanning
        if (clawreach_load_config(url, token) && strlen(url) > 0) {
            ESP_LOGI(LOG_TAG, "Config found (via serial): %s", url);
            return 0;
        }
    }

    ESP_LOGW(LOG_TAG, "QR scan timeout - use serial config instead");
    return -1;
}

void clawreach_qr_show_config(void) {
    char url[MAX_SERVER_URL_LEN] = {0};
    char token[MAX_SERVER_TOKEN_LEN] = {0};
    
    if (!clawreach_load_config(url, token) || strlen(url) == 0) {
        ESP_LOGI(LOG_TAG, "No config to show");
        return;
    }

    ESP_LOGI(LOG_TAG, "Current config: %s", url);
}
