# ESPHome Intercom Integration

This directory contains ESPHome configuration and components for integrating ESP32 devices with the Android WebRTC Intercom system.

## Overview

ESPHome is a system for controlling ESP8266/ESP32 devices using YAML configuration files. This integration allows ESP32 devices running ESPHome to participate in the intercom system.

## Features

- **WebSocket Signaling**: Connects to the same Node-RED signaling server as Android devices
- **Audio I/O**: Uses I2S for microphone input and speaker output
- **RTP Audio Streaming**: Uses UDP/RTP for audio transmission (simpler than full WebRTC)
- **Home Assistant Integration**: Can be controlled via Home Assistant automations

## Hardware Requirements

- ESP32 development board
- I2S microphone (e.g., INMP441)
- I2S amplifier/speaker (e.g., MAX98357A)

### Pin Configuration

Default pin configuration (modify in `intercom_component.cpp` if needed):

**Microphone (INMP441):**
- BCLK: GPIO 32
- WS: GPIO 25
- DATA: GPIO 33

**Speaker (MAX98357A):**
- BCLK: GPIO 26
- LRCLK: GPIO 25
- DIN: GPIO 22

## Installation

### Option 1: ESPHome Add-on (Home Assistant)

1. Install ESPHome add-on in Home Assistant
2. Copy `intercom.yaml` to your ESPHome configuration directory
3. Copy `intercom_component.h` and `intercom_component.cpp` to a custom components directory
4. Create `secrets.yaml` with your WiFi credentials:

```yaml
wifi_ssid: "YOUR_WIFI_SSID"
wifi_password: "YOUR_WIFI_PASSWORD"
api_encryption_key: "YOUR_API_KEY"
ota_password: "YOUR_OTA_PASSWORD"
```

5. Compile and upload to your ESP32 device

### Option 2: Standalone ESPHome

1. Install ESPHome:
   ```bash
   pip install esphome
   ```

2. Create `secrets.yaml` with your credentials

3. Compile and upload:
   ```bash
   esphome compile intercom.yaml
   esphome upload intercom.yaml
   ```

## Configuration

### Basic Configuration

Edit `intercom.yaml` to configure:

- **WiFi**: Set SSID and password in `secrets.yaml`
- **Signaling Server**: Default is `ha.shafferco.com:1880/endpoint/webrtc`
- **Audio Port**: Default UDP port is 5004

### Custom Component

The custom component (`intercom_component.h`/`intercom_component.cpp`) provides:

- WebSocket signaling client
- I2S audio handling
- RTP/UDP audio streaming
- Call state management

## Usage

### Home Assistant Integration

Once installed, the intercom device will appear in Home Assistant with:

- **Switches**: Start Call, End Call, Accept Call
- **Sensors**: WiFi Signal, Uptime
- **Text Sensors**: IP Address, SSID, MAC Address

### Making Calls

1. **Via Home Assistant**:
   - Use the "Start Call" switch
   - Specify target device ID in automation

2. **Via API**:
   ```yaml
   service: esphome.intercom_start_call
   data:
     target_device_id: "station-12345678"
   ```

### Receiving Calls

The device automatically:
- Connects to signaling server on startup
- Joins room with its client ID (format: `esphome-XXXXXXXX`)
- Accepts incoming calls automatically

## Integration with Android

### Current Status

- ✅ Same signaling protocol
- ✅ Same message format
- ⚠️ Different audio protocol (RTP vs WebRTC)

### For Full Interoperability

See `INTEGRATION.md` in the parent directory for options to enable Android ↔ ESPHome calls.

## Troubleshooting

### Device Not Connecting

1. **Check WiFi**: Verify credentials in `secrets.yaml`
2. **Check Signaling Server**: Verify server is accessible
3. **Check Logs**: Enable debug logging in ESPHome

### No Audio

1. **Check I2S Connections**: Verify pin connections
2. **Check Audio Port**: Verify UDP port 5004 is open
3. **Check Buffer Sizes**: May need to adjust for your hardware

### WebSocket Disconnects

1. **Network Stability**: Check WiFi signal strength
2. **Server Availability**: Verify signaling server is running
3. **Reconnect Settings**: Adjust reconnect timeout if needed

## Development

### Custom Component Structure

```
esphome-intercom/
├── intercom.yaml              # Main ESPHome configuration
├── intercom_component.h       # Component header
├── intercom_component.cpp     # Component implementation
└── README.md                  # This file
```

### Adding Features

To extend functionality:

1. **Add Methods**: Add to `intercom_component.h` and implement in `.cpp`
2. **Expose to ESPHome**: Add sensors/switches in `intercom.yaml`
3. **Home Assistant**: Use ESPHome API to control from HA

## Limitations

1. **Audio Protocol**: Uses RTP instead of WebRTC (simpler but requires Android modification)
2. **No Encryption**: Audio is unencrypted (works on local network)
3. **No Echo Cancellation**: Basic audio only
4. **Limited Codecs**: PCM only (no Opus, etc.)

## Future Enhancements

- [ ] Full WebRTC support (if lightweight library available)
- [ ] Audio codec support (Opus)
- [ ] Echo cancellation
- [ ] DTLS-SRTP encryption
- [ ] Call quality metrics
- [ ] Multi-room support

## References

- [ESPHome Documentation](https://esphome.io/)
- [ESPHome Custom Components](https://esphome.io/components/custom.html)
- [ESP32 I2S Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)
- [WebRTC Signaling Protocol](../README.md)

## License

Same as main project.

