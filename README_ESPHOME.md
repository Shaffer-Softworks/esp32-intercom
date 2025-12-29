# ESPHome Integration - Quick Start

This ESPHome component integrates WebRTC intercom functionality with Home Assistant for the Waveshare ESP32-P4-86 panel.

## Quick Setup

1. **Copy the component** to your ESPHome directory:
   ```bash
   cp -r esphome/components/intercom ~/.esphome/components/
   ```

2. **Use the provided configuration**:
   ```bash
   # Copy the Waveshare configuration
   cp esphome/intercom_waveshare.yaml ~/esphome/
   ```

3. **Edit configuration**:
   - Update WiFi credentials in `secrets.yaml`
   - Adjust signaling server settings if needed
   - Customize Home Assistant entity names

4. **Compile and upload**:
   ```bash
   esphome compile intercom_waveshare.yaml
   esphome upload intercom_waveshare.yaml
   ```

## Home Assistant Entities

After installation, you'll have:

- **`sensor.intercom_call_state`** - Call state (0=off, 0.5=ready, 1=in call)
- **`text_sensor.intercom_status`** - Human-readable status
- **`switch.start_intercom_call`** - Start a call (momentary)
- **`switch.end_intercom_call`** - End current call
- **`switch.accept_intercom_call`** - Accept incoming call (momentary)
- **`switch.intercom_mute`** - Mute/unmute audio

## Usage in Home Assistant

### Starting a Call

Use an automation:
```yaml
automation:
  - alias: "Start Intercom Call"
    trigger:
      - platform: state
        entity_id: switch.start_intercom_call
        to: 'on'
    action:
      - service: esphome.waveshare_p4_box_start_call
        data:
          target_device_id: "doorbell-12345678"
      - switch.turn_off: start_intercom_call
```

### Monitoring Call Status

```yaml
template:
  - sensor:
      - name: "Intercom Active"
        state: "{{ 'on' if states('sensor.intercom_call_state') == '1' else 'off' }}"
```

### Audio Integration

The component integrates with your existing audio setup:
- Microphone: Uses `esp32_microphone` at 16kHz
- Speaker: Mixes with other audio sources via `intercom_mixing_input`

## See Also

- [Full Integration Guide](ESPHOME_INTEGRATION.md)
- [Hardware Documentation](WAVESHARE_HARDWARE.md)
- [Migration Guide](MIGRATION.md)

