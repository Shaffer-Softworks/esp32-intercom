# Waveshare ESP32-P4-86 Hardware Configuration

Complete hardware configuration guide for the Waveshare ESP32-P4-86 panel with ES8311 DAC and ES7210 ADC.

## Hardware Components

- **Microcontroller**: ESP32-P4
- **Audio DAC**: ES8311 (I2C address: 0x18)
- **Audio ADC**: ES7210 (I2C address: 0x40)
- **I2S Bus**: Shared between microphone and speaker

## Pin Configuration

### I2S Pins (Shared Bus)
- **MCLK**: GPIO13 - Master clock
- **BCLK**: GPIO12 - Bit clock
- **LRCLK**: GPIO10 - Left/Right clock (Word select)
- **DIN**: GPIO11 - Data input (from ES7210 microphone ADC)
- **DOUT**: GPIO9 - Data output (to ES8311 speaker DAC)

### Control Pins
- **Audio Amplifier**: GPIO53 - Power amplifier enable/disable

### I2C Bus (for ES8311/ES7210 configuration)
- **SDA**: GPIO7
- **SCL**: GPIO8
- **Frequency**: 400kHz
- **ES8311 Address**: 0x18
- **ES7210 Address**: 0x40

## Audio Configuration

### Microphone (ES7210 ADC)
- **Sample Rate**: 16000 Hz
- **Bits per Sample**: 16
- **Channels**: Mono
- **Input**: I2S DIN (GPIO11)

### Speaker (ES8311 DAC)
- **Sample Rate**: 48000 Hz
- **Bits per Sample**: 16
- **Channels**: Mono
- **Output**: I2S DOUT (GPIO9)

## Important Notes

### Sample Rate Mismatch
The microphone operates at 16kHz while the speaker operates at 48kHz. When integrating with WebRTC:

1. **Microphone to WebRTC**: 16kHz is standard for WebRTC audio, so no resampling needed
2. **WebRTC to Speaker**: WebRTC typically outputs at 16kHz or 48kHz. If 16kHz, you'll need to resample to 48kHz for the speaker

### Audio Amplifier
The audio amplifier must be enabled before playing audio and disabled when idle to save power and avoid noise.

### I2C Configuration
The ES8311 DAC and ES7210 ADC are configured via I2C. The code includes `audio_codec.c` which:

1. Initializes I2C bus on GPIO7 (SDA) and GPIO8 (SCL) at 400kHz
2. Configures ES8311 for 48kHz playback
3. Configures ES7210 for 16kHz capture

**Note**: The codec initialization in `audio_codec.c` is a basic implementation. For full functionality, you may need to adjust register values based on the ES8311 and ES7210 datasheets.

## Build Configuration

When building for ESP32-P4:

```bash
idf.py set-target esp32p4
idf.py build
```

## Differences from Standard ESP32

- Uses ESP32-P4 (more powerful, different peripheral configurations)
- Shared I2S bus (not separate RX/TX instances)
- External audio codecs (ES8311/ES7210) instead of direct I2S
- Different GPIO assignments

## References

- [Waveshare ESP32-P4-86 Product Page](https://www.waveshare.com/esp32-p4-86-panel-eth.htm)
- [ES8311 Datasheet](https://www.everest-semi.com/pdf/ES8311%20PB%20V1.0.pdf)
- [ES7210 Datasheet](https://www.everest-semi.com/pdf/ES7210%20PB%20V1.5.pdf)

