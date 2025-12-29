# ESPHome Integration Guide

This guide explains how to integrate the WebRTC Intercom component with ESPHome and Home Assistant.

## Component Structure

The ESPHome component is located in `esphome/components/intercom/`:

```
esphome/components/intercom/
├── __init__.py          # ESPHome component definition
├── intercom.h           # Component header
└── intercom.cpp         # Component implementation
```

## Installation

### Option 1: Local Component (Recommended)

1. Copy the component to your ESPHome components directory:
   ```bash
   cp -r esphome/components/intercom /path/to/esphome/components/
   ```

2. Add to your ESPHome configuration:
   ```yaml
   external_components:
     - source:
         type: local
         path: esphome/components/intercom
   ```

### Option 2: Git Component

1. Push the component to a Git repository
2. Reference it in your ESPHome config:
   ```yaml
   external_components:
     - source:
         type: git
         url: https://github.com/yourusername/esp32-intercom
         path: esphome/components/intercom
   ```

## Configuration

### Basic Configuration

Add to your `esphome` configuration file:

```yaml
intercom:
  id: intercom_device
  signaling_server: "ha.shafferco.com"
  signaling_port: 1880
  signaling_path: "/endpoint/webrtc"
  client_id_prefix: "waveshare-"
```

### Full Configuration with Home Assistant Entities

```yaml
intercom:
  id: intercom_device
  signaling_server: "ha.shafferco.com"
  signaling_port: 1880
  signaling_path: "/endpoint/webrtc"
  client_id_prefix: "waveshare-"
  
  # Sensor: Call state (0=disconnected, 0.5=connected, 1=in call)
  call_state:
    name: "Intercom Call State"
    
  # Text Sensor: Current status
  call_status:
    name: "Intercom Status"
  
  # Switches for call control
  start_call:
    name: "Start Intercom Call"
  end_call:
    name: "End Intercom Call"
  accept_call:
    name: "Accept Intercom Call"
  mute:
    name: "Intercom Mute"
```

## Home Assistant Integration

### Entities Created

When configured, the component creates:

1. **Sensor**: `sensor.intercom_call_state`
   - Values: 0 (disconnected), 0.5 (connected), 1 (in call)

2. **Text Sensor**: `text_sensor.intercom_status`
   - Values: "Disconnected", "Connected", "In Call", etc.

3. **Switches**:
   - `switch.start_intercom_call` - Start a call
   - `switch.end_intercom_call` - End current call
   - `switch.accept_intercom_call` - Accept incoming call
   - `switch.intercom_mute` - Mute/unmute audio

### Home Assistant Automation Example

```yaml
automation:
  # Start a call when doorbell button is pressed
  - alias: "Intercom: Answer Doorbell"
    trigger:
      - platform: state
        entity_id: binary_sensor.doorbell_button
        to: 'on'
    action:
      - service: switch.turn_on
        target:
          entity_id: switch.start_intercom_call
      - service: input_text.set_value
        data:
          entity_id: input_text.intercom_target
          value: "doorbell-12345678"

  # Announce when call ends
  - alias: "Intercom: Call Ended"
    trigger:
      - platform: state
        entity_id: sensor.intercom_call_state
        to: '0.5'
        from: '1'
    action:
      - service: notify.mobile_app
        data:
          message: "Intercom call ended"

  # Mute during media playback
  - alias: "Intercom: Mute During Announcement"
    trigger:
      - platform: state
        entity_id: media_player.waveshare_p4_box
        to: 'playing'
    action:
      - service: switch.turn_on
        target:
          entity_id: switch.intercom_mute
```

### Service Calls

The component also supports direct service calls (via lambdas):

```yaml
# In ESPHome automation
- lambda: |-
    id(intercom_device).start_call("target-device-id");
    id(intercom_device).end_call();
    id(intercom_device).accept_call();
    id(intercom_device).toggle_mute();
```

## Waveshare ESP32-P4-86 Configuration

For the Waveshare hardware, use the provided `intercom_waveshare.yaml` as a template. Key features:

1. **Audio Integration**: Works with existing ES8311/ES7210 setup
2. **ESP-IDF Framework**: Uses ESP-IDF WebSocket client
3. **Audio Mixing**: Can be mixed with other audio sources

## Troubleshooting

### Component Not Found

If ESPHome can't find the component:

1. Check the path in `external_components`
2. Ensure `__init__.py` is present
3. Check ESPHome logs for import errors

### WebSocket Connection Issues

1. Verify signaling server is accessible
2. Check WiFi connection
3. Review component logs: `logger.logs.intercom: debug`

### Audio Issues

1. Ensure I2S audio is configured correctly
2. Verify ES8311 and ES7210 are initialized
3. Check audio amplifier is enabled during calls

## Development

### Testing the Component

1. Enable debug logging:
   ```yaml
   logger:
     logs:
       intercom: debug
   ```

2. Monitor logs:
   ```bash
   esphome logs waveshare-p4-box.yaml
   ```

### Adding Features

To extend the component:

1. Add methods to `intercom.h`
2. Implement in `intercom.cpp`
3. Update `__init__.py` to expose new configuration options
4. Update Home Assistant entities if needed

## Limitations

- **WebRTC Support**: Currently uses simplified signaling. Full WebRTC requires ESP WebRTC Solution integration
- **Audio Codec**: Basic PCM audio. Codec support (Opus) requires additional implementation
- **Multi-call**: Currently supports single call at a time

## Next Steps

1. Complete WebRTC peer connection integration
2. Add audio codec support
3. Implement ICE candidate handling
4. Add call quality metrics

