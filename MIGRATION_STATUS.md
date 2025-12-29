# Migration Status

## ✅ Completed

### 1. ESP-IDF Project Structure
- ✅ Created `CMakeLists.txt` for main project
- ✅ Created `main/CMakeLists.txt` for main component
- ✅ Created `sdkconfig.defaults` with default configuration
- ✅ Updated `.gitignore` for ESP-IDF files

### 2. Core Application Files
- ✅ `main/main.c` - Application entry point
- ✅ `main/intercom_app.c` - Main application logic with WiFi initialization
- ✅ `main/signaling_client.c` - WebSocket signaling client (ESP-IDF)
- ✅ `main/audio_handler.c` - I2S audio capture/playback handler

### 3. Header Files
- ✅ `main/include/intercom_app.h`
- ✅ `main/include/signaling_client.h`
- ✅ `main/include/audio_handler.h`

### 4. Documentation
- ✅ `MIGRATION.md` - Detailed migration guide
- ✅ `README_ESPIDF.md` - Quick start guide for ESP-IDF version
- ✅ `components/README.md` - Instructions for adding ESP WebRTC Solution

## ⚠️ Remaining Tasks

### 1. ESP WebRTC Integration
**Status:** Infrastructure ready, API integration needed

**What's done:**
- Signaling client can send/receive SDP and ICE candidate messages
- Audio handler can capture and playback audio

**What's needed:**
- Integrate `esp_peer` API for WebRTC peer connections
- Handle SDP offer/answer generation and parsing
- Exchange ICE candidates through signaling
- Connect audio streams to ESP WebRTC audio pipeline

**Next steps:**
1. Review `esp-webrtc-solution/solutions/peer_demo` for example usage
2. Initialize `esp_peer` in `intercom_app.c`
3. Implement SDP handling in signaling message callbacks
4. Bridge I2S audio to ESP WebRTC audio streams

### 2. Audio Pipeline Integration
**Status:** I2S handler ready, WebRTC pipeline integration needed

**What's done:**
- I2S microphone capture with callback support
- I2S speaker playback with callback support

**What's needed:**
- Connect captured audio to ESP WebRTC audio encoder
- Connect received WebRTC audio to I2S playback
- Or use `esp_capture` and `av_render` components directly

**Next steps:**
1. Review ESP WebRTC audio pipeline documentation
2. Replace direct I2S callbacks with ESP WebRTC audio streams
3. Or create bridge between I2S and ESP WebRTC audio

## Project Structure

```
esp32-intercom/
├── CMakeLists.txt              ✅
├── sdkconfig.defaults          ✅
├── MIGRATION.md                ✅
├── README_ESPIDF.md            ✅
├── main/
│   ├── CMakeLists.txt          ✅
│   ├── main.c                  ✅
│   ├── intercom_app.c          ✅
│   ├── signaling_client.c      ✅
│   ├── audio_handler.c         ✅
│   └── include/
│       ├── intercom_app.h      ✅
│       ├── signaling_client.h  ✅
│       └── audio_handler.h     ✅
├── components/
│   └── README.md               ✅ (instructions for adding esp-webrtc-solution)
└── [Arduino files]             (original Arduino version preserved)
```

## How to Complete Integration

### Step 1: Add ESP WebRTC Solution

```bash
cd components
git clone --recursive https://github.com/espressif/esp-webrtc-solution.git
```

### Step 2: Update CMakeLists.txt

Once esp-webrtc-solution is added, update `main/CMakeLists.txt` to include the required components:

```cmake
REQUIRES 
    ...
    esp_peer
    esp_capture  # Optional, if using ESP capture instead of direct I2S
    av_render    # Optional, if using ESP render instead of direct I2S
```

### Step 3: Integrate ESP Peer

In `intercom_app.c`, add:

```c
#include "esp_peer.h"

// Initialize peer connection
esp_peer_config_t peer_config = {
    .is_offerer = true,  // or false based on role
};
esp_peer_handle_t peer = esp_peer_create(&peer_config);
```

### Step 4: Handle SDP Messages

In `on_signaling_message()`, add:

```c
if (strcmp(msg->type, "offer") == 0) {
    // Set remote description and create answer
    esp_peer_set_remote_description(peer, msg->sdp);
    char *answer_sdp = esp_peer_create_answer(peer);
    signaling_client_send_answer(answer_sdp);
}
```

### Step 5: Connect Audio

Option A - Use ESP WebRTC audio components:
- Replace I2S direct access with `esp_capture` for input
- Use `av_render` for output

Option B - Bridge I2S to WebRTC:
- Keep I2S handlers
- Connect audio callbacks to ESP WebRTC audio streams

## Testing Checklist

Once integration is complete:

- [ ] WiFi connects successfully
- [ ] Signaling client connects to server
- [ ] Can join room and receive "ready" message
- [ ] SDP offer/answer exchange works
- [ ] ICE candidates are exchanged
- [ ] Audio is captured from microphone
- [ ] Audio is transmitted via WebRTC
- [ ] Audio is received and played on speaker
- [ ] Call can be established with Android device
- [ ] Two-way audio works

## Resources

- [ESP WebRTC Solution GitHub](https://github.com/espressif/esp-webrtc-solution)
- [Peer Demo Example](https://github.com/espressif/esp-webrtc-solution/tree/main/solutions/peer_demo)
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/)

