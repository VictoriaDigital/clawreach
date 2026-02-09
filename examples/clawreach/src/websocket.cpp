/**
 * ClawReach WebSocket Client
 * 
 * Protocol:
 * - Client sends binary frames:
 *   - 0x01 + OPUS audio data
 *   - 0x02 + JPEG frame data
 * - Server sends:
 *   - 0x01 + OPUS audio (TTS response)
 *   - 0x03 + JSON display command
 *   - 0x04 + JSON state update
 */

#include "main.h"

#include <esp_log.h>
#include <esp_websocket_client.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define MSG_TYPE_AUDIO 0x01
#define MSG_TYPE_VIDEO 0x02
#define MSG_TYPE_DISPLAY 0x03
#define MSG_TYPE_STATE 0x04

static esp_websocket_client_handle_t ws_client = NULL;
static bool ws_connected = false;
static SemaphoreHandle_t ws_mutex = NULL;

static char g_server_url[MAX_SERVER_URL_LEN] = {0};
static char g_server_token[MAX_SERVER_TOKEN_LEN] = {0};

// Audio/video send buffers
static uint8_t audio_send_buffer[AUDIO_FRAME_SIZE + 1];
static uint8_t video_send_buffer[JPEG_BUFFER_SIZE + 1];

static void websocket_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data) {
    esp_websocket_event_data_t* data = (esp_websocket_event_data_t*)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(LOG_TAG, "WebSocket connected");
            ws_connected = true;
            ui_listening();
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(LOG_TAG, "WebSocket disconnected");
            ws_connected = false;
            ui_wifi_connecting();  // Show connecting state
            break;

        case WEBSOCKET_EVENT_DATA:
            if (data->data_len > 0 && data->op_code == 0x02) {  // Binary frame
                uint8_t msg_type = ((uint8_t*)data->data_ptr)[0];
                uint8_t* payload = ((uint8_t*)data->data_ptr) + 1;
                size_t payload_len = data->data_len - 1;

                switch (msg_type) {
                    case MSG_TYPE_AUDIO:
                        // TTS audio from server
                        clawreach_audio_decode(payload, payload_len);
                        break;

                    case MSG_TYPE_DISPLAY:
                        // Display command (JSON)
                        ESP_LOGI(LOG_TAG, "Display command: %.*s", 
                                 (int)payload_len, payload);
                        // TODO: Parse JSON and update display
                        break;

                    case MSG_TYPE_STATE:
                        // State update (listening/thinking/speaking)
                        if (payload_len > 0) {
                            if (strncmp((char*)payload, "listening", 9) == 0) {
                                ui_listening();
                            } else if (strncmp((char*)payload, "thinking", 8) == 0) {
                                // Use listening animation for thinking
                                ui_listening();
                            } else if (strncmp((char*)payload, "speaking", 8) == 0) {
                                ui_switch_speaking();
                            }
                        }
                        break;
                }
            }
            break;

        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE(LOG_TAG, "WebSocket error");
            ws_connected = false;
            break;
    }
}

void clawreach_websocket_init(void) {
    ws_mutex = xSemaphoreCreateMutex();
    
    // Load config
    clawreach_load_config(g_server_url, g_server_token);
}

void clawreach_websocket_connect(void) {
    if (strlen(g_server_url) == 0) {
        ESP_LOGE(LOG_TAG, "No server URL configured");
        return;
    }

    esp_websocket_client_config_t ws_cfg = {};
    ws_cfg.uri = g_server_url;
    ws_cfg.buffer_size = WS_BUFFER_SIZE;
    
    // Add auth header if token is set
    static char auth_header[MAX_SERVER_TOKEN_LEN + 32];
    if (strlen(g_server_token) > 0) {
        snprintf(auth_header, sizeof(auth_header), 
                 "Authorization: Bearer %s\r\n", g_server_token);
        ws_cfg.headers = auth_header;
    }

    ws_client = esp_websocket_client_init(&ws_cfg);
    if (ws_client == NULL) {
        ESP_LOGE(LOG_TAG, "Failed to init WebSocket client");
        return;
    }

    esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY,
                                  websocket_event_handler, NULL);

    ESP_LOGI(LOG_TAG, "Connecting to %s", g_server_url);
    esp_err_t err = esp_websocket_client_start(ws_client);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "WebSocket connect failed: %s", esp_err_to_name(err));
    }
}

void clawreach_websocket_loop(void) {
    // The WebSocket client runs in its own task
    // This is just a placeholder for any periodic work
    if (!ws_connected && ws_client != NULL) {
        // Try to reconnect
        static int reconnect_delay = 0;
        if (++reconnect_delay > 500) {  // 5 seconds
            reconnect_delay = 0;
            ESP_LOGI(LOG_TAG, "Attempting reconnect...");
            esp_websocket_client_start(ws_client);
        }
    }
}

void clawreach_send_audio(const uint8_t* data, size_t size) {
    if (!ws_connected || ws_client == NULL) return;
    if (size > AUDIO_FRAME_SIZE) return;

    if (xSemaphoreTake(ws_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        audio_send_buffer[0] = MSG_TYPE_AUDIO;
        memcpy(audio_send_buffer + 1, data, size);
        
        esp_websocket_client_send_bin(ws_client, (char*)audio_send_buffer, 
                                       size + 1, pdMS_TO_TICKS(100));
        xSemaphoreGive(ws_mutex);
    }
}

void clawreach_send_frame(const uint8_t* jpeg, size_t size) {
    if (!ws_connected || ws_client == NULL) return;
    if (size > JPEG_BUFFER_SIZE) return;

    if (xSemaphoreTake(ws_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        video_send_buffer[0] = MSG_TYPE_VIDEO;
        memcpy(video_send_buffer + 1, jpeg, size);
        
        esp_websocket_client_send_bin(ws_client, (char*)video_send_buffer,
                                       size + 1, pdMS_TO_TICKS(500));
        xSemaphoreGive(ws_mutex);
    }
}
