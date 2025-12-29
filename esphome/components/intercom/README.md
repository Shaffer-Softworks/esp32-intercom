# ESPHome Intercom Component

Custom ESPHome component for WebRTC intercom functionality with Android compatibility.

## Quick Setup

### 1. Add ESP WebRTC Solution

#### Option A: As Git Submodule (Recommended)

```bash
cd esphome/components/intercom
git submodule add https://github.com/espressif/esp-webrtc-solution.git esp-webrtc-solution
git submodule update --init --recursive
```

#### Option B: Clone Separately

```bash
cd esphome/components
git clone --recursive https://github.com/espressif/esp-webrtc-solution.git
```

### 2. Verify Configuration

Your ESPHome config must use ESP-IDF framework:

```yaml
esp32:
  framework:
    type: esp-idf
    version: 5.4.2
```

### 3. Build

```bash
esphome compile intercom_waveshare.yaml
```

## Component Features

- WebRTC peer connection via `esp_peer`
- WebSocket signaling
- SDP offer/answer generation
- ICE candidate handling
- Connection state management
- Home Assistant integration

## Documentation

- **[Integration Steps](INTEGRATION_STEPS.md)** - Setup instructions
- **[Main Integration Guide](../../WEBRTC_INTEGRATION.md)** - Complete guide
- **[Calling Guide](../../CALLING_GUIDE.md)** - Usage instructions

