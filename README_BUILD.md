# Build Instructions for Waveshare ESP32-P4-86

## Prerequisites

1. **ESP-IDF v5.4.2** (matching your ESPHome configuration)
   ```bash
   # Install ESP-IDF v5.4.2
   git clone --recursive https://github.com/espressif/esp-idf.git -b v5.4.2
   cd esp-idf
   ./install.sh esp32p4
   . ./export.sh
   ```

2. **ESP WebRTC Solution** (as component)
   ```bash
   cd components
   git clone --recursive https://github.com/espressif/esp-webrtc-solution.git
   ```

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

## Build Steps

1. **Set target**:
   ```bash
   idf.py set-target esp32p4
   ```

2. **Configure** (optional):
   ```bash
   idf.py menuconfig
   ```
   
   Key settings:
   - CPU frequency: 400MHz
   - Flash size: 32MB
   - PSRAM: 200MHz (if available)

3. **Build**:
   ```bash
   idf.py build
   ```

4. **Flash**:
   ```bash
   idf.py -p /dev/ttyUSB0 flash
   ```

5. **Monitor**:
   ```bash
   idf.py -p /dev/ttyUSB0 monitor
   ```

## Hardware Setup

The code is configured for:
- **I2S**: MCLK=GPIO13, BCLK=GPIO12, LRCLK=GPIO10, DIN=GPIO11, DOUT=GPIO9
- **I2C**: SDA=GPIO7, SCL=GPIO8 (400kHz)
- **Audio Amplifier**: GPIO53
- **ES8311 DAC**: I2C address 0x18
- **ES7210 ADC**: I2C address 0x40

## Troubleshooting

### I2C Communication Errors
- Verify I2C connections (SDA=GPIO7, SCL=GPIO8)
- Check that ES8311 and ES7210 are powered
- Verify I2C addresses (0x18 and 0x40)

### Audio Issues
- Verify I2S connections
- Check audio amplifier is enabled (GPIO53)
- Ensure codecs are properly initialized via I2C
- Check sample rates match (mic: 16kHz, speaker: 48kHz)

### WiFi Connection Issues
- Verify SSID and password in `intercom_app.c`
- Check WiFi signal strength
- Verify ESP32-P4 WiFi is working

## Differences from ESPHome

This ESP-IDF implementation:
- Uses direct ESP-IDF APIs instead of ESPHome components
- Requires manual I2C codec configuration
- Uses ESP WebRTC Solution for full WebRTC support
- Provides more control over audio processing

## Next Steps

1. Complete ESP WebRTC integration (see `MIGRATION_STATUS.md`)
2. Test audio capture and playback
3. Test signaling server connection
4. Integrate WebRTC peer connections

