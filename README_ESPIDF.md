# ESP32 Intercom - ESP-IDF Version (WebRTC)

This is the **ESP-IDF version** using the Espressif ESP WebRTC Solution for full WebRTC compatibility with Android devices.

## Overview

This implementation provides:
- ✅ **Full WebRTC Support** - Compatible with Android WebRTC without modifications
- ✅ **DTLS-SRTP Encryption** - Secure audio transmission
- ✅ **WebSocket Signaling** - Same protocol as Android devices
- ✅ **I2S Audio I/O** - Direct microphone and speaker support
- ✅ **ESP-IDF Framework** - More powerful and feature-rich than Arduino

## Quick Start

### Prerequisites

1. **Install ESP-IDF** (v5.0 or later)
   ```bash
   # Follow official ESP-IDF installation guide
   # https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
   ```

2. **Add ESP WebRTC Solution**
   ```bash
   cd components
   git clone --recursive https://github.com/espressif/esp-webrtc-solution.git
   ```

3. **Configure WiFi**
   
   Edit `main/intercom_app.c`:
   ```c
   #define WIFI_SSID "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   ```

4. **Build and Flash**
   ```bash
   idf.py set-target esp32
   idf.py build
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

## Project Structure

```
esp32-intercom/
├── CMakeLists.txt           # Main CMake file
├── sdkconfig.defaults       # Default ESP-IDF configuration
├── main/                    # Main application source
│   ├── CMakeLists.txt
│   ├── main.c              # Entry point
│   ├── intercom_app.c      # Application logic
│   ├── signaling_client.c  # WebSocket signaling
│   ├── audio_handler.c     # I2S audio
│   └── include/            # Headers
├── components/             # External components
│   └── esp-webrtc-solution/ # ESP WebRTC library
└── MIGRATION.md            # Migration guide
```

## Hardware Configuration

Same as Arduino version:
- **I2S Microphone** (INMP441)
  - BCLK: GPIO 32
  - WS: GPIO 25
  - DATA: GPIO 33

- **I2S Speaker** (MAX98357A)
  - BCLK: GPIO 26
  - LRCLK: GPIO 25
  - DIN: GPIO 22

## Configuration

### WiFi Settings

Edit `main/intercom_app.c`:
```c
#define WIFI_SSID "YourNetwork"
#define WIFI_PASSWORD "YourPassword"
```

### Signaling Server

Edit `main/intercom_app.c`:
```c
#define SIGNALING_SERVER "ha.shafferco.com"
#define SIGNALING_PORT 1880
#define SIGNALING_PATH "/endpoint/webrtc"
```

## Current Status

✅ **Completed:**
- ESP-IDF project structure
- WiFi initialization
- WebSocket signaling client
- I2S audio capture/playback
- Basic application framework

⚠️ **In Progress:**
- ESP WebRTC peer connection integration
- SDP offer/answer handling
- ICE candidate exchange
- Audio streaming via WebRTC

## Integration with ESP WebRTC

The code currently includes signaling and audio infrastructure. To complete WebRTC integration:

1. **Initialize ESP Peer** in `intercom_app.c`
2. **Handle SDP messages** from signaling
3. **Exchange ICE candidates**
4. **Connect audio streams** to WebRTC pipeline

See `MIGRATION.md` for detailed integration steps.

## Build Options

```bash
# Set target platform
idf.py set-target esp32        # ESP32
idf.py set-target esp32s3      # ESP32-S3
idf.py set-target esp32c3      # ESP32-C3

# Configure (optional)
idf.py menuconfig

# Build
idf.py build

# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

## Debugging

- Serial output: `idf.py monitor`
- Log levels: Configure in `sdkconfig.defaults`
- Use `ESP_LOGI`, `ESP_LOGW`, `ESP_LOGE` for logging

## Differences from Arduino Version

| Feature | Arduino | ESP-IDF |
|---------|---------|---------|
| Framework | Arduino | ESP-IDF |
| Build System | Arduino IDE | CMake |
| WebRTC | RTP/UDP (simplified) | Full WebRTC |
| Encryption | None | DTLS-SRTP |
| Codecs | PCM only | Multiple codecs |
| Android Compat | Requires mods | Native |

## Resources

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/)
- [ESP WebRTC Solution](https://github.com/espressif/esp-webrtc-solution)
- [Migration Guide](MIGRATION.md)

## License

Same as main project.

