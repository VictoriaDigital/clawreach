# ClawReach

**Physical AI Presence for OpenClaw**

ClawReach gives your AI assistant eyes, ears, and voice in the real world using the SenseCAP Watcher device.

## What is this?

This is a fork of [Seeed Studio's SenseCAP-Watcher-Firmware](https://github.com/Seeed-Studio/SenseCAP-Watcher-Firmware) with a custom `clawreach` example that:

- Streams audio + video to your self-hosted server via WebSocket
- Server runs Whisper (STT) + Vision models + your LLM
- Receives TTS audio responses and display commands
- Privacy-first: all processing on YOUR server, no cloud required

## Hardware

- **Device**: [SenseCAP Watcher W1-A](https://www.seeedstudio.com/SenseCAP-Watcher-W1-A-p-5979.html) (~$50)
- **Capabilities**: Camera, microphone, speaker, 412×412 round LCD, rotary encoder, ESP32-S3 + Himax AI chip

## Quick Start

```bash
# Clone
git clone --recursive https://github.com/VictoriaDigital/clawreach.git
cd clawreach

# Set up ESP-IDF v5.2.1+
. ~/esp/esp-idf/export.sh

# Build ClawReach
cd examples/clawreach
idf.py set-target esp32s3
idf.py build

# Flash
idf.py -p /dev/ttyACM0 flash monitor
```

## Configuration

On first boot, either:

1. **QR Code**: Scan a QR with `{"url":"wss://server/clawreach","token":"..."}`
2. **Serial**: Run `wifi_sta -s SSID -p pass` and `clawreach_server -u wss://...`

## Server

The device connects to your server via WebSocket. See `examples/clawreach/README.md` for the protocol spec.

A reference Python server implementation is planned.

## Examples

| Example | Description |
|---------|-------------|
| `clawreach` | **This one!** WebSocket streaming to self-hosted AI |
| `openai-realtime` | Original Seeed example for OpenAI API |
| `qrcode_reader` | QR code scanning |
| `speech_recognize` | On-device speech recognition |
| `helloworld` | Basic starter |

## License

Apache 2.0 (same as upstream)

## Credits

- [Seeed Studio](https://github.com/Seeed-Studio) for the SenseCAP Watcher and SDK
- [OpenClaw](https://openclaw.ai) for the AI gateway that powers VictorIA
