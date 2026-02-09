/**
 * ClawReach Camera Module
 * 
 * On SenseCAP Watcher, the camera is connected to the Himax WiseEye2 AI chip,
 * not directly to the ESP32-S3. Image capture is handled via the sscma_client.
 * 
 * For initial version, we'll focus on audio streaming.
 * Camera integration via sscma_client will be added later.
 */

#include "main.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// TODO: Integrate with sscma_client for Himax camera capture
// The SenseCAP Watcher camera is managed by the Himax AI chip
// See: sscma_utils_fetch_image_from_reply()

static volatile bool camera_enabled = false;

void clawreach_camera_init(void) {
    ESP_LOGI(LOG_TAG, "Camera init - using Himax AI chip via sscma_client");
    // Camera is managed by Himax chip
    // Full implementation will use sscma_client to request frames
}

int clawreach_camera_capture(uint8_t* buffer, size_t max_size) {
    // TODO: Implement via sscma_client
    // For now, return -1 (no frame)
    return -1;
}

void clawreach_camera_enable(bool enable) {
    camera_enabled = enable;
    ESP_LOGI(LOG_TAG, "Camera %s", enable ? "enabled" : "disabled");
}
