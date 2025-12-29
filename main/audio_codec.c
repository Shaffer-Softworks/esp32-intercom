/*
 * Audio Codec Configuration Implementation
 * ES8311 DAC and ES7210 ADC for Waveshare ESP32-P4-86
 * 
 * Note: This is a basic implementation. Full codec initialization
 * requires writing specific register values. Refer to datasheets:
 * - ES8311: https://www.everest-semi.com/pdf/ES8311%20PB%20V1.0.pdf
 * - ES7210: https://www.everest-semi.com/pdf/ES7210%20PB%20V1.5.pdf
 */

#include "audio_codec.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include <string.h>

static const char *TAG = "audio_codec";

static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t es8311_handle = NULL;
static i2c_master_dev_handle_t es7210_handle = NULL;

// I2C write helper
static esp_err_t i2c_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_master_transmit(dev_handle, data, sizeof(data), -1);
}

// I2C read helper
static esp_err_t i2c_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t *value)
{
    esp_err_t ret = i2c_master_transmit_receive(dev_handle, &reg, 1, value, 1, -1);
    return ret;
}

esp_err_t audio_codec_i2c_init(void)
{
    if (i2c_bus_handle != NULL) {
        ESP_LOGW(TAG, "I2C bus already initialized");
        return ESP_OK;
    }

    // Configure I2C bus
    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };

    esp_err_t ret = i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Add ES8311 device
    i2c_device_config_t es8311_dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ES8311_I2C_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    ret = i2c_master_bus_add_device(i2c_bus_handle, &es8311_dev_config, &es8311_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add ES8311 device: %s", esp_err_to_name(ret));
        return ret;
    }

    // Add ES7210 device
    i2c_device_config_t es7210_dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ES7210_I2C_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    ret = i2c_master_bus_add_device(i2c_bus_handle, &es7210_dev_config, &es7210_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add ES7210 device: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2C bus initialized (SDA=GPIO%d, SCL=GPIO%d, %dkHz)", 
             I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ / 1000);
    ESP_LOGI(TAG, "ES8311 DAC at address 0x%02X", ES8311_I2C_ADDR);
    ESP_LOGI(TAG, "ES7210 ADC at address 0x%02X", ES7210_I2C_ADDR);

    return ESP_OK;
}

esp_err_t audio_codec_es8311_init(uint32_t sample_rate)
{
    if (es8311_handle == NULL) {
        ESP_LOGE(TAG, "ES8311 device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Initializing ES8311 DAC at %dHz", sample_rate);

    // Basic ES8311 initialization
    // Note: This is a simplified initialization. Full initialization requires
    // configuring multiple registers based on the datasheet.
    
    // Power down all blocks first
    i2c_write_reg(es8311_handle, 0x00, 0x7F); // Chip power down
    
    // Clock configuration
    // For 48kHz: MCLK = 256 * FS = 256 * 48000 = 12.288MHz
    // Actual configuration depends on your MCLK source
    
    // I2S configuration
    // Configure for I2S mode, 16-bit, mono
    i2c_write_reg(es8311_handle, 0x17, 0x18); // I2S format: I2S, 16-bit
    
    // DAC configuration
    i2c_write_reg(es8311_handle, 0x18, 0x02); // DAC mute off
    i2c_write_reg(es8311_handle, 0x1F, 0x0C); // DAC volume (adjust as needed)
    
    // Power up DAC
    i2c_write_reg(es8311_handle, 0x00, 0x3C); // Power up DAC
    
    ESP_LOGI(TAG, "ES8311 DAC initialized");
    return ESP_OK;
}

esp_err_t audio_codec_es7210_init(uint32_t sample_rate)
{
    if (es7210_handle == NULL) {
        ESP_LOGE(TAG, "ES7210 device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Initializing ES7210 ADC at %dHz", sample_rate);

    // Basic ES7210 initialization
    // Note: This is a simplified initialization. Full initialization requires
    // configuring multiple registers based on the datasheet.
    
    // Reset
    i2c_write_reg(es7210_handle, 0x00, 0xFF); // Software reset
    
    // Clock configuration
    // For 16kHz: MCLK = 256 * FS = 256 * 16000 = 4.096MHz
    
    // I2S configuration
    i2c_write_reg(es7210_handle, 0x13, 0x10); // I2S format: I2S, 16-bit
    
    // ADC configuration
    i2c_write_reg(es7210_handle, 0x10, 0x41); // Enable ADC, mono
    i2c_write_reg(es7210_handle, 0x11, 0x50); // ADC gain
    
    // Power up
    i2c_write_reg(es7210_handle, 0x00, 0x00); // Power up
    
    ESP_LOGI(TAG, "ES7210 ADC initialized");
    return ESP_OK;
}

esp_err_t audio_codec_es8311_set_volume(uint8_t volume)
{
    if (es8311_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // ES8311 volume control (0-100 -> register value)
    // Volume register is 0x1F (0x00 = mute, 0x33 = max)
    uint8_t vol_reg = (volume * 0x33) / 100;
    
    esp_err_t ret = i2c_write_reg(es8311_handle, 0x1F, vol_reg);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ES8311 volume set to %d%%", volume);
    }
    return ret;
}

esp_err_t audio_codec_es8311_power(bool enable)
{
    if (es8311_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // Power control register (0x00)
    uint8_t reg_val = enable ? 0x3C : 0x7F; // Power up/down
    
    esp_err_t ret = i2c_write_reg(es8311_handle, 0x00, reg_val);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ES8311 %s", enable ? "powered on" : "powered off");
    }
    return ret;
}

esp_err_t audio_codec_es7210_power(bool enable)
{
    if (es7210_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // Power control
    uint8_t reg_val = enable ? 0x00 : 0xFF; // Power up/down
    
    esp_err_t ret = i2c_write_reg(es7210_handle, 0x00, reg_val);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ES7210 %s", enable ? "powered on" : "powered off");
    }
    return ret;
}

