# Migration to ESP WebRTC Solution

This document describes the migration from Arduino-based RTP implementation to ESP-IDF with full WebRTC support using the Espressif ESP WebRTC Solution.

## Architecture Changes

### Before (Arduino)
- WebSocket signaling (WebSockets library)
- RTP/UDP audio streaming
- Arduino framework
- Limited compatibility with Android WebRTC

### After (ESP-IDF + ESP WebRTC)
- WebSocket signaling (ESP-IDF esp_websocket_client)
- Full WebRTC with DTLS-SRTP encryption
- ESP-IDF framework
- Full compatibility with Android WebRTC

## Project Structure

```
esp32-intercom/
├── CMakeLists.txt          # Main project CMake
├── sdkconfig.defaults      # ESP-IDF default config
├── main/                   # Main application
│   ├── CMakeLists.txt
│   ├── main.c              # Application entry point
│   ├── intercom_app.c      # Main application logic
│   ├── signaling_client.c  # WebSocket signaling
│   ├── audio_handler.c     # I2S audio I/O
│   └── include/            # Headers
├── components/             # External components
│   └── esp-webrtc-solution/ # ESP WebRTC library
└── build/                  # Build output (gitignored)
```

## Setup Instructions

### 1. Install ESP-IDF

Follow the [ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/).

```bash
# Clone ESP-IDF
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32

# Set up environment
. ./export.sh
```

### 2. Add ESP WebRTC Solution

```bash
cd /path/to/esp32-intercom
mkdir -p components
cd components
git clone --recursive https://github.com/espressif/esp-webrtc-solution.git
```

### 3. Configure WiFi

Edit `main/intercom_app.c` and update:
- `WIFI_SSID` - Your WiFi network name
- `WIFI_PASSWORD` - Your WiFi password
- `SIGNALING_SERVER` - Your signaling server hostname
- `SIGNALING_PORT` - Signaling server port (default 1880)
- `SIGNALING_PATH` - WebSocket path (default "/endpoint/webrtc")

### 4. Build and Flash

```bash
# Set target (ESP32, ESP32-S3, etc.)
idf.py set-target esp32

# Configure (optional - uses sdkconfig.defaults)
idf.py menuconfig

# Build
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash

# Monitor
idf.py -p /dev/ttyUSB0 monitor
```

## Next Steps: Integrating ESP WebRTC

The current implementation includes:
- ✅ WiFi initialization
- ✅ WebSocket signaling client
- ✅ I2S audio capture/playback
- ⚠️ WebRTC peer connection (needs ESP WebRTC integration)

### Integrating ESP Peer API

To complete the WebRTC integration, you need to:

1. **Initialize ESP Peer**
   ```c
   #include "esp_peer.h"
   
   esp_peer_config_t peer_config = {
       .is_offerer = true,  // or false for answerer
       .ice_servers = NULL, // or configure STUN/TURN servers
   };
   esp_peer_handle_t peer = esp_peer_create(&peer_config);
   ```

2. **Handle SDP Exchange**
   - When receiving "offer" message: Create answer using `esp_peer_create_answer()`
   - When receiving "answer" message: Set remote description using `esp_peer_set_remote_description()`
   - Send generated SDP via signaling client

3. **Handle ICE Candidates**
   - When receiving "candidate" message: Add ICE candidate using `esp_peer_add_ice_candidate()`
   - When ICE candidate is generated: Send via signaling client

4. **Connect Audio to WebRTC**
   - Use `esp_capture` for audio capture (instead of direct I2S)
   - Use `av_render` for audio playback (instead of direct I2S)
   - Or bridge I2S callbacks to ESP WebRTC audio streams

## Key Differences from Arduino Version

1. **Framework**: ESP-IDF instead of Arduino
2. **Build System**: CMake instead of Arduino IDE
3. **Signaling**: ESP-IDF WebSocket client instead of Arduino WebSockets library
4. **Audio**: Will use ESP WebRTC audio pipelines instead of direct RTP/UDP
5. **WebRTC**: Full WebRTC stack with encryption and codecs

## Debugging

- Use `idf.py monitor` to see serial output
- Log levels configured in `sdkconfig.defaults`
- Use `ESP_LOGI`, `ESP_LOGW`, `ESP_LOGE` for logging

## Resources

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [ESP WebRTC Solution](https://github.com/espressif/esp-webrtc-solution)
- [ESP Peer API Documentation](https://github.com/espressif/esp-webrtc-solution/tree/main/components/esp_peer)

