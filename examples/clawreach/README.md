# ClawReach - Physical AI Presence

ClawReach gives your AI assistant (OpenClaw/VictorIA) a physical presence using the SenseCAP Watcher device.

## Features

- **Audio streaming**: Captures audio from mic, sends to server for Whisper STT
- **Video streaming**: Captures camera frames for vision model analysis
- **TTS playback**: Receives and plays audio responses from server
- **Display control**: Server can send display commands (text, images, animations)
- **QR code setup**: Scan a QR code to configure server URL (no hardcoding)
- **Secure connection**: WebSocket with optional auth token

## Hardware

- **Device**: [SenseCAP Watcher W1-A](https://www.seeedstudio.com/SenseCAP-Watcher-W1-A-p-5979.html)
- **MCU**: ESP32-S3
- **AI Chip**: Himax WiseEye2 HX6538
- **Display**: 412×412 round LCD
- **Audio**: Microphone + speaker

## Build

```bash
# Set up ESP-IDF (v5.2.1+)
. ~/esp/esp-idf/export.sh

# Build
cd examples/clawreach
idf.py set-target esp32s3
idf.py build

# Flash
idf.py -p /dev/ttyACM0 flash monitor
```

## Configuration

### Option 1: QR Code (Recommended)
Create a QR code with JSON:
```json
{"url": "wss://your-server.com/clawreach", "token": "optional_auth_token"}
```

Point the camera at the QR code on first boot.

### Option 2: Serial Console
```bash
# Set WiFi
wifi_sta -s YourSSID -p YourPassword

# Set server
clawreach_server -u wss://your-server.com/clawreach -t optional_token

# Reboot to apply
reboot
```

## Protocol

### Client → Server (Binary WebSocket)

| Byte 0 | Payload | Description |
|--------|---------|-------------|
| 0x01   | OPUS    | Audio frame (60ms @ 16kHz) |
| 0x02   | JPEG    | Camera frame (QVGA) |

### Server → Client (Binary WebSocket)

| Byte 0 | Payload | Description |
|--------|---------|-------------|
| 0x01   | OPUS    | TTS audio response |
| 0x03   | JSON    | Display command |
| 0x04   | String  | State: "listening", "thinking", "speaking" |

## Server Setup

See the [ClawReach Server](../../../server/) directory for a Python reference implementation using:
- **Whisper**: Speech-to-text
- **Vision model**: Image understanding  
- **Your LLM**: Generate response
- **TTS**: Text-to-speech

## Roadmap

- [x] WebSocket streaming (v1 - current)
- [ ] WebRTC option for lower latency (~50ms vs ~150ms)
- [ ] Camera streaming via sscma_client
- [ ] QR code config scanning
- [ ] Display command parsing (images, text, animations)

## License

Apache 2.0
