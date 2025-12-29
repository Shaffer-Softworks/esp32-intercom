/*
 * Audio Handler
 * I2S audio capture and playback for WebRTC
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// I2S Pin Configuration for Waveshare ESP32-P4-86
// Shared I2S bus for microphone and speaker
#define I2S_MCLK_PIN GPIO_NUM_13
#define I2S_LRCLK_PIN GPIO_NUM_10
#define I2S_BCLK_PIN GPIO_NUM_12
#define I2S_DIN_PIN GPIO_NUM_11  // Microphone input
#define I2S_DOUT_PIN GPIO_NUM_9  // Speaker output

// Audio amplifier control
#define AUDIO_AMP_PIN GPIO_NUM_53

// Audio Configuration
#define MIC_SAMPLE_RATE 16000    // Microphone sample rate
#define SPEAKER_SAMPLE_RATE 48000 // Speaker sample rate
#define BITS_PER_SAMPLE 16
#define CHANNELS 1
#define BUFFER_SIZE 1024

typedef void (*audio_capture_cb_t)(int16_t *data, size_t samples, void *user_data);
typedef void (*audio_playback_cb_t)(int16_t *data, size_t samples, void *user_data);

/**
 * @brief Initialize I2S audio for Waveshare ESP32-P4-86
 * 
 * Configures shared I2S bus for ES8311 DAC (speaker) and ES7210 ADC (microphone)
 */
esp_err_t audio_handler_init(void);

/**
 * @brief Set audio capture callback
 */
void audio_handler_set_capture_cb(audio_capture_cb_t cb, void *user_data);

/**
 * @brief Set audio playback callback
 */
void audio_handler_set_playback_cb(audio_playback_cb_t cb, void *user_data);

/**
 * @brief Start audio capture
 */
esp_err_t audio_handler_start_capture(void);

/**
 * @brief Stop audio capture
 */
esp_err_t audio_handler_stop_capture(void);

/**
 * @brief Start audio playback
 */
esp_err_t audio_handler_start_playback(void);

/**
 * @brief Stop audio playback
 */
esp_err_t audio_handler_stop_playback(void);

/**
 * @brief Process audio (call from main loop)
 */
void audio_handler_process(void);

/**
 * @brief Enable/disable audio amplifier
 */
esp_err_t audio_handler_set_amplifier(bool enable);

/**
 * @brief Deinitialize audio handler
 */
void audio_handler_deinit(void);

#ifdef __cplusplus
}
#endif

