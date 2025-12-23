# Hardware Setup Guide

This guide explains how to connect the required hardware components for the ESP32 Intercom.

## Required Components

1. **ESP32 Development Board**
   - ESP32, ESP32-S3, or compatible
   - USB cable for programming

2. **I2S Microphone**
   - Recommended: INMP441
   - Alternative: SPH0645LM4H, ICS-43434

3. **I2S Amplifier/Speaker**
   - Recommended: MAX98357A
   - Alternative: PCM5102A (DAC only, needs external amplifier)

4. **Speaker**
   - 4-8 ohm speaker (if using MAX98357A)
   - Or headphones with amplifier

5. **Power Supply**
   - USB power (5V) or external 5V supply
   - Ensure adequate current capacity (500mA+)

## Pin Connections

### Default Pin Configuration

The default pin configuration in the code:

```
ESP32          INMP441          MAX98357A
------         ------           ---------
3.3V    -----> VDD
GND     -----> GND              GND
GPIO 32 -----> SCK              (not used)
GPIO 25 -----> WS               LRCLK
GPIO 33 -----> SD               (not used)
GPIO 26 -----> (not used)       BCLK
GPIO 22 -----> (not used)       DIN
5V      -----> (not used)       VDD
```

### Detailed Connections

#### INMP441 Microphone

| INMP441 Pin | ESP32 Pin | Description |
|-------------|-----------|-------------|
| VDD         | 3.3V      | Power (3.3V) |
| GND         | GND       | Ground |
| SCK         | GPIO 32   | Serial Clock |
| WS          | GPIO 25   | Word Select (Left/Right) |
| SD          | GPIO 33   | Serial Data |

**Note:** INMP441 is a 3.3V device. Do not connect to 5V!

#### MAX98357A Amplifier

| MAX98357A Pin | ESP32 Pin | Description |
|---------------|-----------|-------------|
| VIN           | 5V        | Power (5V) |
| GND           | GND       | Ground |
| BCLK          | GPIO 26   | Bit Clock |
| LRCLK         | GPIO 25   | Left/Right Clock |
| DIN           | GPIO 22   | Data Input |

**Speaker Connection:**
- Connect 4-8 ohm speaker between `+` and `-` terminals on MAX98357A

## Alternative Hardware

### Using Different I2S Pins

If your hardware requires different pins, modify these defines in `esp32_intercom.ino`:

```cpp
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_32
#define I2S_MIC_LEFT_RIGHT GPIO_NUM_25
#define I2S_MIC_SERIAL_DATA GPIO_NUM_33

#define I2S_SPEAKER_SERIAL_CLOCK GPIO_NUM_26
#define I2S_SPEAKER_LEFT_RIGHT GPIO_NUM_25
#define I2S_SPEAKER_SERIAL_DATA GPIO_NUM_22
```

### Using Different Microphones

#### SPH0645LM4H

Same pinout as INMP441, but may require different I2S configuration:
- Check datasheet for specific requirements
- May need pull-up resistors on some pins

#### Analog Microphone + ADC

If using an analog microphone:
- Use ESP32 ADC pins
- Requires different code (not I2S)
- Lower quality, simpler setup

### Using Different Amplifiers

#### PCM5102A (DAC Only)

- Requires external amplifier
- Connect to amplifier input
- No built-in amplification

#### External I2S DAC

- Use appropriate I2S pins
- May require different configuration
- Check datasheet for requirements

## Power Considerations

### Current Requirements

- ESP32: ~80-240mA (depending on WiFi usage)
- INMP441: ~1-2mA
- MAX98357A: ~20-50mA (depends on volume)
- **Total: ~100-300mA**

### Power Supply Options

1. **USB Power (Recommended)**
   - 5V from USB port
   - Most convenient
   - Limited to ~500mA (USB 2.0)

2. **External 5V Supply**
   - Use quality 5V supply
   - Ensure adequate current capacity
   - Add decoupling capacitors if needed

3. **Battery Power**
   - Use 3.7V LiPo with 5V boost converter
   - Consider power management
   - Monitor battery level

## Troubleshooting

### No Audio Input

1. **Check Connections**
   - Verify all pins are connected correctly
   - Check for loose connections

2. **Check Power**
   - Verify 3.3V to microphone
   - Check current draw

3. **Check I2S Configuration**
   - Verify pin numbers in code
   - Check sample rate matches hardware

4. **Test Microphone**
   - Try different microphone
   - Check microphone datasheet

### No Audio Output

1. **Check Connections**
   - Verify speaker connections
   - Check amplifier power (5V)

2. **Check Volume**
   - MAX98357A has no volume control in code
   - May need hardware volume control

3. **Check Speaker**
   - Test speaker with other source
   - Verify impedance (4-8 ohm)

4. **Check I2S Configuration**
   - Verify pin numbers
   - Check sample rate

### Distorted Audio

1. **Power Supply**
   - Insufficient current
   - Poor power supply quality
   - Add decoupling capacitors

2. **Ground Issues**
   - Ensure common ground
   - Check for ground loops

3. **Sample Rate**
   - Mismatch between code and hardware
   - Try different sample rates

4. **Buffer Size**
   - Too small buffers cause dropouts
   - Too large buffers cause latency
   - Adjust `BUFFER_SIZE` in code

## Advanced Configuration

### Custom Pin Layout

For custom pin layouts, modify the I2S pin configuration:

```cpp
i2s_pin_config_t i2s_mic_pins = {
  .bck_io_num = YOUR_BCLK_PIN,
  .ws_num = YOUR_WS_PIN,
  .data_in_num = YOUR_DATA_PIN,
  .data_out_num = I2S_PIN_NO_CHANGE
};
```

### Multiple Audio Channels

For stereo or multi-channel audio:
- Modify `channel_format` in I2S config
- Update buffer handling code
- Check hardware supports multiple channels

### Higher Sample Rates

To use higher sample rates (e.g., 48kHz):
- Update `SAMPLE_RATE` define
- Ensure hardware supports it
- May require more processing power
- Larger buffers may be needed

## Safety Notes

1. **Voltage Levels**
   - INMP441 is 3.3V - do not exceed!
   - MAX98357A is 5V tolerant
   - Check all component voltage ratings

2. **Current Limits**
   - Don't exceed ESP32 current limits
   - Use appropriate power supply
   - Add fuses if needed

3. **Heat**
   - MAX98357A can get warm
   - Ensure adequate ventilation
   - Don't block heat dissipation

4. **ESD Protection**
   - Handle components carefully
   - Use ESD-safe workspace
   - Ground yourself before handling

## Resources

- [ESP32 I2S Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)
- [INMP441 Datasheet](https://www.invensense.com/products/digital/inmp441/)
- [MAX98357A Datasheet](https://datasheets.maximintegrated.com/en/ds/MAX98357A-MAX98357B.pdf)

