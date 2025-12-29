/*
 * Intercom Application
 * Main application entry point for ESP32 Intercom
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the intercom application
 * 
 * Initializes WiFi, WebRTC, signaling, and audio components
 */
void intercom_app_start(void);

#ifdef __cplusplus
}
#endif

