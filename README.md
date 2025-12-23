# ESP32 Intercom Client

ESP32 implementation of the intercom system, compatible with the Android WebRTC Intercom.

## Overview

This ESP32 intercom client connects to the same signaling server as Android devices and enables voice communication between ESP32 devices and Android tablets/phones.

## Features

- ✅ **WebSocket Signaling** - Same protocol as Android, connects to Node-RED signaling server
- ✅ **I2S Audio I/O** - Microphone input and speaker output
- ✅ **RTP/UDP Audio Streaming** - Simplified audio protocol (full WebRTC is too resource-intensive)
- ✅ **Auto-Reconnect** - Automatically reconnects to signaling server
- ✅ **Always-On Mode** - Stays connected to receive incoming calls
- ✅ **Call Management** - Start, accept, and end calls

## Hardware Requirements

- ESP32 development board (ESP32, ESP32-S3, etc.)
- I2S microphone (e.g., INMP441)
- I2S amplifier/speaker (e.g., MAX98357A)

### Pin Configuration

Default pin configuration (modify in code if needed):

**Microphone (INMP441):**
- SCK (Serial Clock): GPIO 32
- WS (Word Select): GPIO 25
- SD (Serial Data): GPIO 33

**Speaker (MAX98357A):**
- BCLK (Bit Clock): GPIO 26
- LRCLK (Left/Right Clock): GPIO 25
- DIN (Data Input): GPIO 22

## Software Requirements

### Arduino IDE

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - File → Preferences → Additional Board Manager URLs
   - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Boards Manager → Search "ESP32" → Install

### Required Libraries

Install via Arduino Library Manager (Sketch → Include Library → Manage Libraries):

1. **WebSockets** by Markus Sattler
   - Search: "WebSockets"
   - Install: "WebSockets" by Markus Sattler

2. **ArduinoJson** by Benoit Blanchon
   - Search: "ArduinoJson"
   - Install: "ArduinoJson" by Benoit Blanchon

## Installation

1. **Clone this repository:**
   ```bash
   git clone <repository-url>
   cd esp32-intercom-repo
   ```

2. **Open `esp32_intercom.ino` in Arduino IDE**

3. **Configure settings:**
   - Edit WiFi credentials:
     ```cpp
     const char* ssid = "YOUR_WIFI_SSID";
     const char* password = "YOUR_WIFI_PASSWORD";
     ```
   - Edit signaling server (if different):
     ```cpp
     const char* signalingServer = "ha.shafferco.com";
     const int signalingPort = 1880;
     const char* signalingPath = "/endpoint/webrtc";
     ```

4. **Select board:**
   - Tools → Board → ESP32 Arduino → Your ESP32 board

5. **Upload:**
   - Connect ESP32 via USB
   - Tools → Port → Select your COM port
   - Click Upload

## Usage

### Automatic Operation

Once uploaded, the ESP32 will:
1. Connect to WiFi
2. Connect to signaling server
3. Join room with its client ID (format: `esp32-XXXXXXXX`)
4. Wait for incoming calls or can initiate calls

### Making Calls

Currently, calls are initiated programmatically. To add call initiation:

1. Modify the code to call `startCall(targetDeviceId)` when needed
2. Or use serial commands (see Serial Commands section)

### Receiving Calls

The device automatically:
- Accepts incoming calls
- Routes audio through I2S
- Handles call termination

## Serial Commands

Connect via Serial Monitor (115200 baud) for debugging:

- Monitor connection status
- View signaling messages
- Debug audio issues

## Configuration

### Audio Settings

```cpp
#define SAMPLE_RATE 16000        // Audio sample rate (Hz)
#define BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
#define I2S_CHANNELS I2S_CHANNEL_MONO
#define BUFFER_SIZE 1024         // Audio buffer size
```

### Network Settings

```cpp
const char* signalingServer = "ha.shafferco.com";
const int signalingPort = 1880;
const char* signalingPath = "/endpoint/webrtc";
int localAudioPort = 5004;      // UDP port for audio
```

## Signaling Protocol

The ESP32 uses the same signaling message format as Android:

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

ESP32 devices use: `esp32-XXXXXXXX`
- Where `XXXXXXXX` is derived from MAC address
- Example: `esp32-12345678`

## Audio Format

- **Sample Rate**: 16 kHz
- **Bit Depth**: 16-bit
- **Channels**: Mono
- **Protocol**: RTP/UDP (simplified, not full WebRTC)

## Integration with Android

### Current Status

- ✅ Same signaling protocol
- ✅ Same message format
- ⚠️ Different audio protocol (RTP vs WebRTC)

### For Full Interoperability

The Android app currently only supports WebRTC. To enable Android ↔ ESP32 calls:

1. **Option 1**: Modify Android app to support RTP for ESP32 devices
   - Detect ESP32 devices (clientId starts with "esp32-")
   - Use RTP/UDP for ESP32 calls
   - Keep WebRTC for Android-to-Android calls

2. **Option 2**: WebRTC Bridge Server
   - Server converts WebRTC ↔ RTP
   - No Android changes needed

3. **Option 3**: Lightweight WebRTC on ESP32
   - Implement minimal WebRTC subset
   - Most complex option

## Troubleshooting

### Device Not Connecting

1. **Check WiFi**: Verify credentials are correct
2. **Check Signaling Server**: Verify server is accessible
3. **Check Serial Monitor**: Look for error messages

### No Audio Output

1. **Check I2S Connections**: Verify pin connections match code
2. **Check Audio Port**: Verify UDP port 5004 is open
3. **Check Buffer Sizes**: May need adjustment for your hardware

### WebSocket Disconnects

1. **Network Stability**: Check WiFi signal strength
2. **Server Availability**: Verify signaling server is running
3. **Reconnect**: Device auto-reconnects every 5 seconds

## Development

### Project Structure

```
esp32-intercom-repo/
├── esp32_intercom.ino    # Main Arduino sketch
├── README.md              # This file
├── LICENSE                # License file
├── .gitignore            # Git ignore file
└── docs/                  # Additional documentation
    ├── HARDWARE.md       # Hardware setup guide
    └── INTEGRATION.md    # Integration guide
```

### Adding Features

To extend functionality:

1. **Add Methods**: Add to the main sketch
2. **Modify Signaling**: Update `handleSignalingMessage()`
3. **Audio Processing**: Modify `sendAudioPacket()` / `receiveAudioPacket()`

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
- [ ] Serial command interface for call control

## License

[Specify your license here]

## Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## References

- [WebRTC Specification](https://www.w3.org/TR/webrtc/)
- [RTP Specification](https://tools.ietf.org/html/rfc3550)
- [ESP32 I2S Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)
- [Arduino WebSockets Library](https://github.com/Links2004/arduinoWebSockets)

## Support

For issues and questions:
- Open an issue on GitHub
- Check the documentation in `docs/`
- Review the troubleshooting section

