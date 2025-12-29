/*
 * Audio Codec Configuration
 * ES8311 DAC and ES7210 ADC initialization for Waveshare ESP32-P4-86
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// I2C Configuration (from ESPHome config)
#define I2C_SDA_PIN GPIO_NUM_7
#define I2C_SCL_PIN GPIO_NUM_8
#define I2C_FREQ_HZ 400000  // 400kHz
#define ES8311_I2C_ADDR 0x18
#define ES7210_I2C_ADDR 0x40

/**
 * @brief Initialize I2C bus for audio codecs
 */
esp_err_t audio_codec_i2c_init(void);

/**
 * @brief Initialize ES8311 DAC (speaker)
 * @param sample_rate Sample rate in Hz (e.g., 48000)
 */
esp_err_t audio_codec_es8311_init(uint32_t sample_rate);

/**
 * @brief Initialize ES7210 ADC (microphone)
 * @param sample_rate Sample rate in Hz (e.g., 16000)
 */
esp_err_t audio_codec_es7210_init(uint32_t sample_rate);

/**
 * @brief Set ES8311 volume
 * @param volume Volume level (0-100)
 */
esp_err_t audio_codec_es8311_set_volume(uint8_t volume);

/**
 * @brief Enable/disable ES8311 DAC
 */
esp_err_t audio_codec_es8311_power(bool enable);

/**
 * @brief Enable/disable ES7210 ADC
 */
esp_err_t audio_codec_es7210_power(bool enable);

#ifdef __cplusplus
}
#endif

