/*
 * Signaling Client
 * WebSocket-based signaling client for WebRTC
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SIGNALING_STATE_DISCONNECTED,
    SIGNALING_STATE_CONNECTING,
    SIGNALING_STATE_CONNECTED,
    SIGNALING_STATE_JOINED,
    SIGNALING_STATE_READY,
} signaling_state_t;

typedef struct {
    char *type;
    char *roomId;
    char *clientId;
    char *sessionId;
    char *sdp;
    char *candidate;
    char *message;
} signaling_message_t;

typedef void (*signaling_message_cb_t)(signaling_message_t *msg, void *user_data);
typedef void (*signaling_state_cb_t)(signaling_state_t state, void *user_data);

/**
 * @brief Initialize signaling client
 * 
 * @param server Signaling server hostname
 * @param port Signaling server port
 * @param path WebSocket path
 * @param client_id Client ID for this device
 * @return esp_err_t 
 */
esp_err_t signaling_client_init(const char *server, int port, const char *path, const char *client_id);

/**
 * @brief Set message callback
 */
void signaling_client_set_message_cb(signaling_message_cb_t cb, void *user_data);

/**
 * @brief Set state callback
 */
void signaling_client_set_state_cb(signaling_state_cb_t cb, void *user_data);

/**
 * @brief Connect to signaling server
 */
esp_err_t signaling_client_connect(void);

/**
 * @brief Disconnect from signaling server
 */
esp_err_t signaling_client_disconnect(void);

/**
 * @brief Join a room
 */
esp_err_t signaling_client_join(const char *room_id, const char *session_id);

/**
 * @brief Send offer
 */
esp_err_t signaling_client_send_offer(const char *sdp);

/**
 * @brief Send answer
 */
esp_err_t signaling_client_send_answer(const char *sdp);

/**
 * @brief Send ICE candidate
 */
esp_err_t signaling_client_send_candidate(const char *candidate);

/**
 * @brief Send leave message
 */
esp_err_t signaling_client_send_leave(void);

/**
 * @brief Process signaling events (call from main loop)
 */
void signaling_client_process(void);

/**
 * @brief Cleanup signaling client
 */
void signaling_client_deinit(void);

#ifdef __cplusplus
}
#endif

