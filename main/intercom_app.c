/*
 * Intercom Application Implementation
 * Main application logic integrating WebRTC, signaling, and audio
 */

#include "intercom_app.h"
#include "signaling_client.h"
#include "audio_handler.h"
#include "audio_codec.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

// Audio sample rates (from audio_handler.h)
#define MIC_SAMPLE_RATE 16000
#define SPEAKER_SAMPLE_RATE 48000

static const char *TAG = "intercom_app";

// Configuration - TODO: Move to Kconfig or NVS
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define SIGNALING_SERVER "ha.shafferco.com"
#define SIGNALING_PORT 1880
#define SIGNALING_PATH "/endpoint/webrtc"

// Application state
static char client_id[32];
static char session_id[64];
static char current_room_id[32];
static bool is_in_call = false;
static bool is_muted = false;

// Generate client ID from MAC address
static void generate_client_id(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(client_id, sizeof(client_id), "esp32-%02X%02X%02X%02X", 
             mac[2], mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "Client ID: %s", client_id);
}

// Generate session ID
static void generate_session_id(void)
{
    uint32_t random1 = esp_random();
    uint32_t random2 = (uint32_t)esp_timer_get_time();
    snprintf(session_id, sizeof(session_id), "%08X%08X", random1, random2);
    ESP_LOGI(TAG, "Session ID: %s", session_id);
}

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        
        // Initialize signaling after WiFi is connected
        if (signaling_client_init(SIGNALING_SERVER, SIGNALING_PORT, SIGNALING_PATH, client_id) == ESP_OK) {
            signaling_client_connect();
        }
    }
}

// Initialize WiFi
static esp_err_t init_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_sta());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization finished.");
    return ESP_OK;
}

// Signaling message callback
static void on_signaling_message(signaling_message_t *msg, void *user_data)
{
    if (strcmp(msg->type, "joined") == 0) {
        ESP_LOGI(TAG, "Joined room: %s", msg->roomId);
        // Send ready message after joining
    } else if (strcmp(msg->type, "ready") == 0) {
        ESP_LOGI(TAG, "Room is ready");
        // Start WebRTC peer connection
    } else if (strcmp(msg->type, "offer") == 0) {
        ESP_LOGI(TAG, "Received offer");
        // Handle WebRTC offer
        // TODO: Create answer using esp_peer API
    } else if (strcmp(msg->type, "answer") == 0) {
        ESP_LOGI(TAG, "Received answer");
        // Handle WebRTC answer
        is_in_call = true;
        // Enable audio amplifier when call starts
        audio_handler_set_amplifier(true);
        audio_handler_start_capture();
        audio_handler_start_playback();
    } else if (strcmp(msg->type, "candidate") == 0) {
        ESP_LOGI(TAG, "Received ICE candidate");
        // Handle ICE candidate
    } else if (strcmp(msg->type, "leave") == 0) {
        ESP_LOGI(TAG, "Remote left");
        is_in_call = false;
        // Disable audio amplifier when call ends
        audio_handler_stop_capture();
        audio_handler_stop_playback();
        audio_handler_set_amplifier(false);
    }
}

// Signaling state callback
static void on_signaling_state(signaling_state_t state, void *user_data)
{
    switch (state) {
        case SIGNALING_STATE_CONNECTED:
            ESP_LOGI(TAG, "Signaling connected");
            generate_session_id();
            strcpy(current_room_id, client_id);
            signaling_client_join(current_room_id, session_id);
            break;
        case SIGNALING_STATE_READY:
            ESP_LOGI(TAG, "Signaling ready");
            break;
        default:
            break;
    }
}

// Main application task
static void intercom_task(void *pvParameters)
{
    while (1) {
        // Process signaling events
        signaling_client_process();
        
        // Process audio
        if (is_in_call) {
            audio_handler_process();
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void intercom_app_start(void)
{
    ESP_LOGI(TAG, "Starting Intercom Application...");
    
    // Generate client ID
    generate_client_id();
    
    // Initialize I2C for audio codecs
    if (audio_codec_i2c_init() == ESP_OK) {
        // Initialize ES8311 DAC (speaker) at 48kHz
        audio_codec_es8311_init(SPEAKER_SAMPLE_RATE);
        // Initialize ES7210 ADC (microphone) at 16kHz
        audio_codec_es7210_init(MIC_SAMPLE_RATE);
    } else {
        ESP_LOGW(TAG, "I2C codec initialization failed, continuing without codec config");
    }
    
    // Initialize audio handler (I2S)
    audio_handler_init();
    
    // Initialize WiFi
    init_wifi();
    
    // Set up signaling callbacks
    signaling_client_set_message_cb(on_signaling_message, NULL);
    signaling_client_set_state_cb(on_signaling_state, NULL);
    
    // Start main application task
    xTaskCreate(intercom_task, "intercom_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Intercom Application Started");
}

