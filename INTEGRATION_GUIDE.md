# ESPHome Intercom Integration Guide

This guide explains how to integrate ESPHome devices with the Android WebRTC Intercom system.

## Architecture

```
┌─────────────┐         ┌──────────────┐         ┌─────────────┐
│   Android  │◄───────►│   Signaling  │◄───────►│  ESPHome    │
│   Device   │ WebRTC  │    Server    │  RTP    │   Device    │
│            │         │  (Node-RED)  │         │             │
└─────────────┘         └──────────────┘         └─────────────┘
```

## Setup Instructions

### 1. Install ESPHome

**Option A: Home Assistant Add-on**
- Install ESPHome add-on from Home Assistant Supervisor
- Access ESPHome dashboard

**Option B: Standalone**
```bash
pip install esphome
```

### 2. Create Configuration

1. Copy `intercom.yaml` to your ESPHome config directory
2. Create `secrets.yaml` with your WiFi credentials:
   ```yaml
   wifi_ssid: "YOUR_WIFI_SSID"
   wifi_password: "YOUR_WIFI_PASSWORD"
   api_encryption_key: "YOUR_API_KEY"
   ota_password: "YOUR_OTA_PASSWORD"
   ```

### 3. Install Custom Component

The custom component needs to be in the ESPHome components directory:

**For Home Assistant:**
- Copy `intercom_component.h` and `intercom_component.cpp` to:
  ```
  /config/esphome/components/intercom/
  ```

**For Standalone:**
- Create a `components/intercom/` directory in your ESPHome config
- Copy the component files there

### 4. Compile and Upload

**Via Home Assistant:**
- Open ESPHome dashboard
- Click "Install" on your device
- Select "Plug into this computer" or "Wirelessly"

**Via Command Line:**
```bash
esphome compile intercom.yaml
esphome upload intercom.yaml
```

## Component Configuration

### Basic Configuration

```yaml
intercom:
  id: intercom_device
  signaling_server: "ha.shafferco.com"
  signaling_port: 1880
  signaling_path: "/endpoint/webrtc"
  audio_port: 5004
```

### Advanced Configuration

```yaml
intercom:
  id: intercom_device
  signaling_server: "your-server.com"
  signaling_port: 1880
  signaling_path: "/endpoint/webrtc"
  audio_port: 5004
  
  # I2S Pin Configuration (optional, defaults shown)
  i2s_mic_bclk: GPIO32
  i2s_mic_ws: GPIO25
  i2s_mic_data: GPIO33
  i2s_spk_bclk: GPIO26
  i2s_spk_ws: GPIO25
  i2s_spk_data: GPIO22
```

## Home Assistant Integration

Once installed, the device will appear in Home Assistant with:

### Entities

- **Switches:**
  - `intercom.start_call` - Start a call
  - `intercom.end_call` - End current call
  - `intercom.accept_call` - Accept incoming call
  - `intercom.mute` - Toggle mute

- **Sensors:**
  - `intercom.wifi_signal` - WiFi signal strength
  - `intercom.uptime` - Device uptime

- **Binary Sensors:**
  - `intercom.in_call` - Call state
  - `intercom.connected` - Signaling connection state

- **Text Sensors:**
  - `intercom.client_id` - Device client ID
  - `intercom.ip_address` - Device IP address

### Automations

**Example: Start call from Home Assistant**
```yaml
automation:
  - alias: "Call Front Door"
    trigger:
      - platform: state
        entity_id: input_button.front_door_call
    action:
      - service: esphome.intercom_start_call
        data:
          target_device_id: "station-12345678"
```

**Example: Answer call automatically**
```yaml
automation:
  - alias: "Auto Answer Intercom"
    trigger:
      - platform: state
        entity_id: binary_sensor.intercom_in_call
        to: 'on'
    condition:
      - condition: state
        entity_id: input_boolean.auto_answer_intercom
        state: 'on'
    action:
      - service: esphome.intercom_accept_call
```

## API Usage

### ESPHome API

The component exposes methods via ESPHome API:

```python
# Python example
import esphome.api_client

client = esphome.api_client.APIClient("192.168.1.100", 6053)

# Start a call
client.call_service("intercom", "start_call", {
    "target_device_id": "station-12345678"
})

# End call
client.call_service("intercom", "end_call", {})

# Toggle mute
client.call_service("intercom", "toggle_mute", {})
```

### HTTP API

ESPHome also exposes HTTP API:

```bash
# Start call
curl -X POST http://192.168.1.100:6053/intercom/start_call \
  -H "Content-Type: application/json" \
  -d '{"target_device_id": "station-12345678"}'

# End call
curl -X POST http://192.168.1.100:6053/intercom/end_call
```

## Signaling Protocol

The ESPHome device uses the same signaling protocol as Android:

### Message Format

```json
{
  "type": "join",
  "roomId": "esphome-12345678",
  "clientId": "esphome-12345678",
  "sessionId": "abc123..."
}
```

### Message Types

- `join` - Join a room
- `joined` - Confirmation of joining
- `ready` - Room is ready (2 peers)
- `offer` - Call offer (SDP)
- `answer` - Call answer (SDP)
- `candidate` - ICE candidate
- `leave` - Leave call
- `error` - Error message

### Client ID Format

ESPHome devices use the format: `esphome-XXXXXXXX`
- Where `XXXXXXXX` is derived from MAC address
- Example: `esphome-12345678`

## Audio Configuration

### I2S Setup

The component uses I2S for audio I/O:

- **Sample Rate**: 16 kHz
- **Bit Depth**: 16-bit
- **Channels**: Mono
- **Protocol**: RTP/UDP (simplified, not full WebRTC)

### Hardware Connections

**INMP441 Microphone:**
- VDD → 3.3V
- GND → GND
- SCK → GPIO 32
- WS → GPIO 25
- SD → GPIO 33

**MAX98357A Amplifier:**
- VDD → 5V
- GND → GND
- BCLK → GPIO 26
- LRCLK → GPIO 25
- DIN → GPIO 22

## Troubleshooting

### Device Not Connecting

1. **Check WiFi**: Verify credentials in `secrets.yaml`
2. **Check Logs**: Enable debug logging:
   ```yaml
   logger:
     level: DEBUG
   ```
3. **Check Network**: Verify device can reach signaling server

### No Audio

1. **Check I2S Connections**: Verify pin connections
2. **Check Audio Port**: Verify UDP port 5004 is open
3. **Check Buffer Sizes**: May need adjustment for hardware

### WebSocket Disconnects

1. **Network Stability**: Check WiFi signal strength
2. **Server Availability**: Verify signaling server is running
3. **Reconnect Settings**: Component auto-reconnects every 5 seconds

### Component Not Found

1. **Check Path**: Verify component files are in correct location
2. **Check Syntax**: Verify YAML syntax is correct
3. **Check Logs**: Look for component loading errors

## Development

### Customizing the Component

To modify behavior:

1. **Edit `intercom_component.cpp`**:
   - Modify signaling logic
   - Adjust audio handling
   - Add features

2. **Recompile**:
   ```bash
   esphome compile intercom.yaml
   ```

3. **Upload**:
   ```bash
   esphome upload intercom.yaml
   ```

### Adding Features

Example: Add volume control

1. **Add to `intercom_component.h`**:
   ```cpp
   void set_volume(float volume);
   ```

2. **Implement in `intercom_component.cpp`**:
   ```cpp
   void IntercomComponent::set_volume(float volume) {
     // Implementation
   }
   ```

3. **Expose in `intercom.yaml`**:
   ```yaml
   number:
     - platform: intercom
       name: "Volume"
       min_value: 0
       max_value: 100
       step: 1
   ```

## Limitations

1. **Audio Protocol**: Uses RTP instead of WebRTC (requires Android modification for full interoperability)
2. **No Encryption**: Audio is unencrypted (works on local network)
3. **No Echo Cancellation**: Basic audio only
4. **Limited Codecs**: PCM only

## Next Steps

For full Android ↔ ESPHome interoperability:

1. **Option 1**: Modify Android app to support RTP for ESPHome devices
2. **Option 2**: Create WebRTC bridge server
3. **Option 3**: Implement lightweight WebRTC on ESPHome

See `../INTEGRATION.md` for details.

