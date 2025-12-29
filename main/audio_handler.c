/*
 * Audio Handler Implementation
 * I2S audio capture and playback for Waveshare ESP32-P4-86
 * 
 * Hardware:
 * - ES8311 DAC (speaker) via I2C address 0x18
 * - ES7210 ADC (microphone) via I2C address 0x40
 * - Shared I2S bus
 * - Audio amplifier controlled via GPIO53
 */

#include "audio_handler.h"
#include "esp_log.h"
#include "driver/i2s.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "audio_handler";

// Audio handler state
static struct {
    bool initialized;
    bool capture_active;
    bool playback_active;
    bool amplifier_enabled;
    audio_capture_cb_t capture_cb;
    void *capture_user_data;
    audio_playback_cb_t playback_cb;
    void *playback_user_data;
    TaskHandle_t capture_task_handle;
    TaskHandle_t playback_task_handle;
} s_audio = {0};

// Audio capture task
// Note: Microphone uses ES7210 ADC at 16kHz
static void audio_capture_task(void *pvParameters)
{
    int16_t *buffer = (int16_t *)malloc(BUFFER_SIZE * sizeof(int16_t));
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate capture buffer");
        vTaskDelete(NULL);
        return;
    }

    while (s_audio.capture_active) {
        size_t bytes_read = 0;
        // Read from shared I2S bus (DIN pin)
        esp_err_t ret = i2s_read(I2S_NUM_0, buffer, BUFFER_SIZE * sizeof(int16_t), &bytes_read, portMAX_DELAY);
        
        if (ret == ESP_OK && bytes_read > 0) {
            size_t samples = bytes_read / sizeof(int16_t);
            if (s_audio.capture_cb) {
                s_audio.capture_cb(buffer, samples, s_audio.capture_user_data);
            }
        } else if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S read error: %s", esp_err_to_name(ret));
        }
    }

    free(buffer);
    vTaskDelete(NULL);
}

// Audio playback task
// Note: Speaker uses ES8311 DAC at 48kHz
// Input audio should be resampled from 16kHz to 48kHz if needed
static void audio_playback_task(void *pvParameters)
{
    int16_t *buffer = (int16_t *)malloc(BUFFER_SIZE * sizeof(int16_t));
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate playback buffer");
        vTaskDelete(NULL);
        return;
    }

    while (s_audio.playback_active) {
        if (s_audio.playback_cb) {
            // Get audio data from callback (expected at 48kHz)
            s_audio.playback_cb(buffer, BUFFER_SIZE, s_audio.playback_user_data);
            
            // Write to shared I2S bus (DOUT pin) - ES8311 DAC
            size_t bytes_written = 0;
            esp_err_t ret = i2s_write(I2S_NUM_0, buffer, BUFFER_SIZE * sizeof(int16_t), &bytes_written, portMAX_DELAY);
            
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "I2S write error: %s", esp_err_to_name(ret));
            }
        } else {
            // If no callback, just delay
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    free(buffer);
    vTaskDelete(NULL);
}

esp_err_t audio_handler_init(void)
{
    if (s_audio.initialized) {
        ESP_LOGW(TAG, "Audio handler already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Configure audio amplifier GPIO
    gpio_config_t amp_gpio_config = {
        .pin_bit_mask = (1ULL << AUDIO_AMP_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&amp_gpio_config);
    gpio_set_level(AUDIO_AMP_PIN, 0); // Start with amplifier off
    s_audio.amplifier_enabled = false;
    ESP_LOGI(TAG, "Audio amplifier GPIO configured (GPIO%d)", AUDIO_AMP_PIN);

    // Configure shared I2S bus for duplex mode (RX + TX)
    // Note: ES8311 DAC and ES7210 ADC share the same I2S bus
    // We configure for duplex mode - speaker at 48kHz, mic at 16kHz
    // In practice, we'll configure for 48kHz and resample mic input, or use separate I2S instances
    
    // For now, configure I2S_NUM_0 in duplex mode at 48kHz (speaker rate)
    // Microphone will be resampled from 16kHz to 48kHz if needed
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
        .sample_rate = SPEAKER_SAMPLE_RATE, // 48kHz for ES8311 DAC
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // Mono
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = BUFFER_SIZE,
        .use_apll = true, // Use APLL for better clock quality
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0,
    };

    // Shared I2S pin configuration for Waveshare ESP32-P4-86
    i2s_pin_config_t i2s_pins = {
        .mck_io_num = I2S_MCLK_PIN,      // GPIO13
        .bck_io_num = I2S_BCLK_PIN,      // GPIO12
        .ws_io_num = I2S_LRCLK_PIN,      // GPIO10
        .data_out_num = I2S_DOUT_PIN,    // GPIO9 - ES8311 DAC input
        .data_in_num = I2S_DIN_PIN,      // GPIO11 - ES7210 ADC output
    };

    esp_err_t ret = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2s_set_pin(I2S_NUM_0, &i2s_pins);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pins: %s", esp_err_to_name(ret));
        i2s_driver_uninstall(I2S_NUM_0);
        return ret;
    }

    i2s_zero_dma_buffer(I2S_NUM_0);

    s_audio.initialized = true;
    ESP_LOGI(TAG, "Audio handler initialized for Waveshare ESP32-P4-86");
    ESP_LOGI(TAG, "I2S: MCLK=GPIO%d, BCLK=GPIO%d, LRCLK=GPIO%d", I2S_MCLK_PIN, I2S_BCLK_PIN, I2S_LRCLK_PIN);
    ESP_LOGI(TAG, "Microphone (ES7210): DIN=GPIO%d, Sample rate=%dHz", I2S_DIN_PIN, MIC_SAMPLE_RATE);
    ESP_LOGI(TAG, "Speaker (ES8311): DOUT=GPIO%d, Sample rate=%dHz", I2S_DOUT_PIN, SPEAKER_SAMPLE_RATE);
    
    return ESP_OK;
}

void audio_handler_set_capture_cb(audio_capture_cb_t cb, void *user_data)
{
    s_audio.capture_cb = cb;
    s_audio.capture_user_data = user_data;
}

void audio_handler_set_playback_cb(audio_playback_cb_t cb, void *user_data)
{
    s_audio.playback_cb = cb;
    s_audio.playback_user_data = user_data;
}

esp_err_t audio_handler_start_capture(void)
{
    if (!s_audio.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_audio.capture_active) {
        ESP_LOGW(TAG, "Capture already active");
        return ESP_ERR_INVALID_STATE;
    }

    s_audio.capture_active = true;
    xTaskCreate(audio_capture_task, "audio_capture", 4096, NULL, 5, &s_audio.capture_task_handle);

    ESP_LOGI(TAG, "Audio capture started");
    return ESP_OK;
}

esp_err_t audio_handler_stop_capture(void)
{
    if (!s_audio.capture_active) {
        return ESP_ERR_INVALID_STATE;
    }

    s_audio.capture_active = false;
    if (s_audio.capture_task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100)); // Wait for task to finish
        s_audio.capture_task_handle = NULL;
    }

    ESP_LOGI(TAG, "Audio capture stopped");
    return ESP_OK;
}

esp_err_t audio_handler_start_playback(void)
{
    if (!s_audio.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_audio.playback_active) {
        ESP_LOGW(TAG, "Playback already active");
        return ESP_ERR_INVALID_STATE;
    }

    s_audio.playback_active = true;
    xTaskCreate(audio_playback_task, "audio_playback", 4096, NULL, 5, &s_audio.playback_task_handle);

    ESP_LOGI(TAG, "Audio playback started");
    return ESP_OK;
}

esp_err_t audio_handler_stop_playback(void)
{
    if (!s_audio.playback_active) {
        return ESP_ERR_INVALID_STATE;
    }

    s_audio.playback_active = false;
    if (s_audio.playback_task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100)); // Wait for task to finish
        s_audio.playback_task_handle = NULL;
    }

    ESP_LOGI(TAG, "Audio playback stopped");
    return ESP_OK;
}

void audio_handler_process(void)
{
    // Audio processing is handled in separate tasks
    // This can be used for periodic tasks if needed
}

esp_err_t audio_handler_set_amplifier(bool enable)
{
    if (!s_audio.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    gpio_set_level(AUDIO_AMP_PIN, enable ? 1 : 0);
    s_audio.amplifier_enabled = enable;
    ESP_LOGI(TAG, "Audio amplifier %s", enable ? "enabled" : "disabled");
    return ESP_OK;
}

void audio_handler_deinit(void)
{
    audio_handler_stop_capture();
    audio_handler_stop_playback();
    audio_handler_set_amplifier(false);

    if (s_audio.initialized) {
        i2s_stop(I2S_NUM_0);
        i2s_driver_uninstall(I2S_NUM_0);
        s_audio.initialized = false;
    }

    ESP_LOGI(TAG, "Audio handler deinitialized");
}

