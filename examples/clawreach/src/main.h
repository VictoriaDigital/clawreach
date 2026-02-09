#pragma once

#include "sensecap-watcher.h"

extern "C" {
#include "ui.h"
}

#define LOG_TAG "ClawReach"

// ClawReach server config
#define CLAWREACH_NVS_NAMESPACE "clawreach"
#define CLAWREACH_SERVER_URL_KEY "server_url"
#define CLAWREACH_SERVER_TOKEN_KEY "server_token"

#define MAX_SERVER_URL_LEN 256
#define MAX_SERVER_TOKEN_LEN 128

// Buffer sizes
#define AUDIO_FRAME_SIZE 960  // 60ms at 16kHz
#define JPEG_BUFFER_SIZE (32 * 1024)  // 32KB for JPEG frames
#define WS_BUFFER_SIZE (64 * 1024)  // 64KB WebSocket buffer

// UI functions (from ui.c)
// ui_init() - already in ui.h
// ui_listening() - already in ui.h
// ui_switch_speaking() - for TTS playback

// Console init
int cmd_init(void);

// WiFi (from wifi.cpp - using oai_ names for compatibility)
void oai_wifi_init(void);
void oai_wifi(void);

// Audio (from media.cpp)
void clawreach_init_audio_capture(void);
void clawreach_init_audio_encoder(void);
void clawreach_init_audio_decoder(void);
void clawreach_audio_decode(uint8_t* data, size_t size);
void clawreach_capture_and_send_audio(void);

// Camera
void clawreach_camera_init(void);
int clawreach_camera_capture(uint8_t* buffer, size_t max_size);

// WebSocket
void clawreach_websocket_init(void);
void clawreach_websocket_connect(void);
void clawreach_websocket_loop(void);
void clawreach_send_audio(const uint8_t* data, size_t size);
void clawreach_send_frame(const uint8_t* jpeg, size_t size);

// Config
int clawreach_qr_scan_config(void);
int clawreach_load_config(char* url_out, char* token_out);
int clawreach_save_config(const char* url, const char* token);
