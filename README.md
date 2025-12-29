# ESP32 Intercom - ESPHome & Home Assistant Integration

WebRTC intercom system for ESP32 devices with full Android WebRTC compatibility via ESPHome and Home Assistant integration.

## Overview

This project provides:
- **ESPHome Custom Component** for WebRTC intercom functionality
- **Full WebRTC Support** using Espressif ESP WebRTC Solution
- **Home Assistant Integration** with sensors, switches, and automations
- **Android Compatibility** - Works with standard Android WebRTC devices
- **Waveshare ESP32-P4-86 Support** - Optimized for ESP32-P4 panel hardware

## Features

- ✅ **WebRTC Signaling** - WebSocket-based signaling compatible with Android
- ✅ **Full WebRTC Peer Connection** - DTLS-SRTP encryption, ICE candidate handling
- ✅ **Audio I/O** - I2S audio with ES8311 DAC and ES7210 ADC support
- ✅ **Home Assistant Entities** - Switches, sensors, and text sensors
- ✅ **Auto-Connect** - Automatic call establishment
- ✅ **Auto-Accept** - Automatic incoming call acceptance
- ✅ **Call Management** - Start, end, accept, and mute controls

## Quick Start

1. **Install ESPHome Component:**
   ```bash
   cp -r esphome/components/intercom ~/.esphome/components/
   ```

2. **Add ESP WebRTC Solution:**
   ```bash
   cd ~/.esphome/components/intercom
   git submodule add https://github.com/espressif/esp-webrtc-solution.git esp-webrtc-solution
   cd esp-webrtc-solution && git submodule update --init --recursive
   ```

3. **Use Configuration:**
   - Copy `esphome/intercom_waveshare.yaml` to your ESPHome directory
   - Update WiFi credentials in `secrets.yaml`
   - Compile and upload

4. **Configure in Home Assistant:**
   - Set target device ID in `input_text.intercom_target_device_id`
   - Use `switch.start_intercom_call` to initiate calls
   - Monitor status via sensors

## Documentation

- **[ESPHome Quick Start](README_ESPHOME.md)** - Quick setup and installation
- **[WebRTC Integration Guide](WEBRTC_INTEGRATION.md)** - ESP WebRTC Solution setup
- **[Calling Guide](CALLING_GUIDE.md)** - How to make and receive calls
- **[ESPHome Integration](ESPHOME_INTEGRATION.md)** - Detailed integration guide
- **[Waveshare Hardware](WAVESHARE_HARDWARE.md)** - Waveshare ESP32-P4-86 configuration
- **[General Hardware](docs/HARDWARE.md)** - General hardware setup guide

## Hardware Support

### Waveshare ESP32-P4-86 Panel

- **Microcontroller**: ESP32-P4
- **Audio DAC**: ES8311 (I2C 0x18)
- **Audio ADC**: ES7210 (I2C 0x40)
- **I2S**: Shared bus for microphone and speaker
- **Sample Rates**: 16kHz (mic), 48kHz (speaker)

See [WAVESHARE_HARDWARE.md](WAVESHARE_HARDWARE.md) for pin configurations and setup.

## Project Structure

```
esp32-intercom/
├── esphome/
│   ├── components/intercom/      # ESPHome custom component
│   └── intercom_waveshare.yaml   # Complete configuration
├── main/                          # ESP-IDF version (alternative)
├── docs/                          # Additional documentation
└── README.md                      # This file
```

## Requirements

- **ESPHome** with ESP-IDF framework (v5.4.2+)
- **ESP WebRTC Solution** (as submodule)
- **ESP32-P4** or compatible ESP32 variant
- **Home Assistant** (optional, for automation)

## Android Compatibility

✅ **Fully Compatible** - Uses standard WebRTC with:
- DTLS-SRTP encryption
- ICE candidate processing
- Proper SDP generation
- Standard WebRTC media streaming

Works with unmodified Android WebRTC applications.

## License

See LICENSE file.
