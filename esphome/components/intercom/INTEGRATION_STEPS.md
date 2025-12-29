# ESP WebRTC Solution Setup Steps

Quick reference for adding ESP WebRTC Solution to the intercom component.

## Prerequisites

- ESPHome with ESP-IDF framework (v5.4.2+)
- Git for submodules

## Step 1: Add ESP WebRTC Solution

### Option A: Git Submodule (Recommended)

```bash
cd esphome/components/intercom
git submodule add https://github.com/espressif/esp-webrtc-solution.git esp-webrtc-solution
cd esp-webrtc-solution
git submodule update --init --recursive
```

### Option B: Clone Separately

```bash
cd esphome/components
git clone --recursive https://github.com/espressif/esp-webrtc-solution.git
```

## Build and Test

```bash
esphome compile intercom_waveshare.yaml
esphome upload intercom_waveshare.yaml
```

If you get build errors about missing components:
1. Verify ESP WebRTC Solution is cloned with submodules
2. Check ESP-IDF version matches (5.4.2)
3. Verify component paths

## Next Steps

After building successfully:
1. Connect audio pipeline to WebRTC (see `WEBRTC_INTEGRATION.md`)
2. Test with Android device
3. Verify audio flows correctly

For detailed information, see:
- [WEBRTC_INTEGRATION.md](../../WEBRTC_INTEGRATION.md) - Complete integration guide
- [README.md](README.md) - Component documentation

