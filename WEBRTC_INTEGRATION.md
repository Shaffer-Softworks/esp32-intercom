# ESP WebRTC Solution Integration

Complete guide for integrating Espressif ESP WebRTC Solution into the ESPHome intercom component.

## Overview

The ESPHome intercom component uses Espressif's ESP WebRTC Solution to provide full WebRTC compatibility with Android devices. This includes DTLS-SRTP encryption, ICE candidate processing, and standard WebRTC media streaming.

## Status

✅ **WebRTC Integration Complete** (in code)
⚠️ **Setup Required** - ESP WebRTC Solution must be added

## Quick Setup

### 1. Add ESP WebRTC Solution

```bash
cd ~/.esphome/components/intercom
git submodule add https://github.com/espressif/esp-webrtc-solution.git esp-webrtc-solution
cd esp-webrtc-solution
git submodule update --init --recursive
```

### 2. Verify ESP-IDF Framework

Your ESPHome configuration must use ESP-IDF framework:

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

## What's Integrated

### WebRTC Peer Connection

- ✅ `esp_peer` API integration
- ✅ Peer creation (offerer/answerer)
- ✅ SDP offer/answer generation
- ✅ Remote description handling
- ✅ ICE candidate processing

### Signaling Integration

- ✅ WebSocket signaling
- ✅ SDP exchange via signaling
- ✅ ICE candidate relay
- ✅ Connection state management

### Connection Flow

1. **Initiate Call**: Creates WebRTC peer as offerer
2. **Send Offer**: Generates WebRTC SDP offer
3. **Receive Answer**: Sets remote description
4. **ICE Candidates**: Exchanges via signaling
5. **DTLS Handshake**: Automatic via ESP WebRTC
6. **Media Streaming**: Audio via WebRTC RTP/SRTP

## Remaining Work

### Audio Pipeline Integration ⚠️

The audio callbacks are currently placeholders. You need to connect ESPHome's audio components to WebRTC:

**Option 1: Use ESP WebRTC Audio Components**
- Use `esp_capture` for microphone input
- Use `av_render` for speaker output

**Option 2: Bridge ESPHome Audio**
- Connect ESPHome microphone to WebRTC capture callback
- Connect WebRTC render callback to ESPHome speaker

**Current Status:** Audio callbacks (`audio_capture_cb`, `audio_render_cb`) need implementation.

## Testing Checklist

After setup:

- [ ] ESP WebRTC Solution compiles
- [ ] WebRTC peer creates successfully
- [ ] SDP offers generated correctly
- [ ] ICE candidates exchanged
- [ ] DTLS handshake completes
- [ ] Connection reaches CONNECTED state
- [ ] Audio flows (after audio integration)
- [ ] Works with Android device

## Troubleshooting

### Build: Missing esp_peer.h

**Solution:**
1. Verify ESP WebRTC Solution is cloned with submodules
2. Check component paths
3. Verify ESP-IDF version

### Runtime: Peer Creation Fails

**Solution:**
1. Check ESP-IDF version compatibility
2. Verify memory is sufficient
3. Check logs for specific errors

### Connection: No Audio

**Solution:**
1. Audio pipeline needs to be connected
2. Verify audio callbacks are implemented
3. Check WebRTC connection state

## API Reference

The integration uses ESP WebRTC Solution APIs:

- `esp_peer_create()` - Create peer connection
- `esp_peer_create_offer()` - Generate offer
- `esp_peer_create_answer()` - Generate answer
- `esp_peer_set_remote_description()` - Set remote SDP
- `esp_peer_add_ice_candidate()` - Add ICE candidate

**Note:** Verify actual API signatures in ESP WebRTC Solution documentation. The implementation may need adjustment based on the actual API.

## Resources

- [ESP WebRTC Solution](https://github.com/espressif/esp-webrtc-solution)
- [Peer Demo Example](https://github.com/espressif/esp-webrtc-solution/tree/main/solutions/peer_demo)
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/)
