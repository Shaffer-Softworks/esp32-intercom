/*
 * Signaling Client Implementation
 * WebSocket-based signaling using ESP-IDF
 */

#include "signaling_client.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_websocket_client.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "signaling";

// Signaling client state
typedef struct {
    char server[128];
    int port;
    char path[128];
    char client_id[32];
    signaling_state_t state;
    signaling_message_cb_t message_cb;
    void *message_user_data;
    signaling_state_cb_t state_cb;
    void *state_user_data;
    esp_websocket_client_handle_t websocket_handle;
    bool connected;
} signaling_client_t;

static signaling_client_t s_client = {0};

// Helper to set state and notify callback
static void set_state(signaling_state_t state)
{
    if (s_client.state != state) {
        s_client.state = state;
        if (s_client.state_cb) {
            s_client.state_cb(state, s_client.state_user_data);
        }
    }
}

// Parse signaling message from JSON
static void parse_signaling_message(const char *json_str, signaling_message_t *msg)
{
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return;
    }

    cJSON *type = cJSON_GetObjectItem(json, "type");
    cJSON *roomId = cJSON_GetObjectItem(json, "roomId");
    cJSON *clientId = cJSON_GetObjectItem(json, "clientId");
    cJSON *sessionId = cJSON_GetObjectItem(json, "sessionId");
    cJSON *sdp = cJSON_GetObjectItem(json, "sdp");
    cJSON *candidate = cJSON_GetObjectItem(json, "candidate");
    cJSON *message = cJSON_GetObjectItem(json, "message");

    if (type && cJSON_IsString(type)) msg->type = cJSON_GetStringValue(type);
    if (roomId && cJSON_IsString(roomId)) msg->roomId = cJSON_GetStringValue(roomId);
    if (clientId && cJSON_IsString(clientId)) msg->clientId = cJSON_GetStringValue(clientId);
    if (sessionId && cJSON_IsString(sessionId)) msg->sessionId = cJSON_GetStringValue(sessionId);
    if (sdp && cJSON_IsString(sdp)) msg->sdp = cJSON_GetStringValue(sdp);
    if (candidate && cJSON_IsString(candidate)) msg->candidate = cJSON_GetStringValue(candidate);
    if (message && cJSON_IsString(message)) msg->message = cJSON_GetStringValue(message);

    cJSON_Delete(json);
}

// WebSocket event handler
static void websocket_event_handler(void *handler_args, esp_event_base_t base,
                                   int32_t event_id, void *event_data)
{
    esp_websocket_event_id_t ws_event_id = (esp_websocket_event_id_t)event_id;
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (ws_event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WebSocket Connected");
            s_client.connected = true;
            set_state(SIGNALING_STATE_CONNECTED);
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "WebSocket Disconnected");
            s_client.connected = false;
            set_state(SIGNALING_STATE_DISCONNECTED);
            break;

        case WEBSOCKET_EVENT_DATA:
            if (data->op_code == 0x08 && data->data_len == 2) {
                ESP_LOGI(TAG, "Received closed message");
            } else {
                ESP_LOGI(TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
                signaling_message_t msg = {0};
                parse_signaling_message((char *)data->data_ptr, &msg);
                if (s_client.message_cb) {
                    s_client.message_cb(&msg, s_client.message_user_data);
                }
            }
            break;

        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE(TAG, "WebSocket error");
            break;

        default:
            break;
    }
}

esp_err_t signaling_client_init(const char *server, int port, const char *path, const char *client_id)
{
    if (!server || !path || !client_id) {
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(s_client.server, server, sizeof(s_client.server) - 1);
    s_client.port = port;
    strncpy(s_client.path, path, sizeof(s_client.path) - 1);
    strncpy(s_client.client_id, client_id, sizeof(s_client.client_id) - 1);
    s_client.state = SIGNALING_STATE_DISCONNECTED;
    s_client.connected = false;

    ESP_LOGI(TAG, "Signaling client initialized: %s:%d%s", server, port, path);
    return ESP_OK;
}

void signaling_client_set_message_cb(signaling_message_cb_t cb, void *user_data)
{
    s_client.message_cb = cb;
    s_client.message_user_data = user_data;
}

void signaling_client_set_state_cb(signaling_state_cb_t cb, void *user_data)
{
    s_client.state_cb = cb;
    s_client.state_user_data = user_data;
}

esp_err_t signaling_client_connect(void)
{
    if (s_client.websocket_handle) {
        ESP_LOGW(TAG, "WebSocket already connected");
        return ESP_ERR_INVALID_STATE;
    }

    char uri[256];
    snprintf(uri, sizeof(uri), "ws://%s:%d%s", s_client.server, s_client.port, s_client.path);

    esp_websocket_client_config_t websocket_cfg = {
        .uri = uri,
    };

    esp_websocket_client_handle_t client = esp_websocket_client_init(&websocket_cfg);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize WebSocket client");
        return ESP_FAIL;
    }

    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);
    esp_err_t ret = esp_websocket_client_start(client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WebSocket client");
        esp_websocket_client_destroy(client);
        return ret;
    }

    s_client.websocket_handle = client;
    set_state(SIGNALING_STATE_CONNECTING);
    return ESP_OK;
}

esp_err_t signaling_client_disconnect(void)
{
    if (s_client.websocket_handle) {
        esp_websocket_client_stop(s_client.websocket_handle);
        esp_websocket_client_destroy(s_client.websocket_handle);
        s_client.websocket_handle = NULL;
        s_client.connected = false;
        set_state(SIGNALING_STATE_DISCONNECTED);
    }
    return ESP_OK;
}

static esp_err_t send_json_message(cJSON *json)
{
    if (!s_client.websocket_handle || !s_client.connected) {
        ESP_LOGE(TAG, "WebSocket not connected");
        return ESP_ERR_INVALID_STATE;
    }

    char *json_str = cJSON_PrintUnformatted(json);
    if (!json_str) {
        ESP_LOGE(TAG, "Failed to serialize JSON");
        return ESP_FAIL;
    }

    esp_err_t ret = esp_websocket_client_send_text(
        s_client.websocket_handle,
        json_str, strlen(json_str), portMAX_DELAY);

    free(json_str);
    return ret;
}

esp_err_t signaling_client_join(const char *room_id, const char *session_id)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "type", "join");
    cJSON_AddStringToObject(json, "roomId", room_id);
    cJSON_AddStringToObject(json, "clientId", s_client.client_id);
    cJSON_AddStringToObject(json, "sessionId", session_id);

    esp_err_t ret = send_json_message(json);
    cJSON_Delete(json);

    if (ret == ESP_OK) {
        set_state(SIGNALING_STATE_JOINED);
    }
    return ret;
}

esp_err_t signaling_client_send_offer(const char *sdp)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "type", "offer");
    cJSON_AddStringToObject(json, "sdp", sdp);

    esp_err_t ret = send_json_message(json);
    cJSON_Delete(json);
    return ret;
}

esp_err_t signaling_client_send_answer(const char *sdp)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "type", "answer");
    cJSON_AddStringToObject(json, "sdp", sdp);

    esp_err_t ret = send_json_message(json);
    cJSON_Delete(json);
    return ret;
}

esp_err_t signaling_client_send_candidate(const char *candidate)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "type", "candidate");
    cJSON_AddStringToObject(json, "candidate", candidate);

    esp_err_t ret = send_json_message(json);
    cJSON_Delete(json);
    return ret;
}

esp_err_t signaling_client_send_leave(void)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "type", "leave");

    esp_err_t ret = send_json_message(json);
    cJSON_Delete(json);
    return ret;
}

void signaling_client_process(void)
{
    // WebSocket processing is handled by ESP-IDF internally
    // This can be used for periodic tasks if needed
}

void signaling_client_deinit(void)
{
    signaling_client_disconnect();
    memset(&s_client, 0, sizeof(s_client));
}

