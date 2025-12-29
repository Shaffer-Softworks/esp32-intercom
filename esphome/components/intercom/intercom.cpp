#include "intercom.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/wifi/wifi_component.h"

namespace esphome {
namespace intercom {

static const char *TAG = "intercom";

#ifndef USE_ESP_IDF
IntercomComponent *IntercomComponent::instance_ = nullptr;
#endif

void IntercomComponent::setup() {
#ifdef USE_ESP_IDF
  ESP_LOGCONFIG(TAG, "Setting up Intercom Component (ESP-IDF)...");
#else
  instance_ = this;
  ESP_LOGCONFIG(TAG, "Setting up Intercom Component (Arduino)...");
#endif
  
  generate_client_id_();
  ESP_LOGCONFIG(TAG, "Client ID: %s", client_id_.c_str());
  
  // Connect to signaling server after WiFi is ready
  this->set_timeout(2000, [this]() {
    this->connect_websocket();
  });
}

void IntercomComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Intercom Component:");
  ESP_LOGCONFIG(TAG, "  Signaling Server: %s:%d%s", signaling_server_.c_str(), signaling_port_, signaling_path_.c_str());
  ESP_LOGCONFIG(TAG, "  Client ID: %s", client_id_.c_str());
}

void IntercomComponent::loop() {
#ifdef USE_ESP_IDF
  // WebSocket processing is handled by ESP-IDF internally
#else
  web_socket_.loop();
#endif
  
  // Handle switch actions
  if (start_call_switch_ && start_call_switch_->state) {
    // Start call action would be triggered via automation
    // This is a placeholder - actual call target should come from automation
    ESP_LOGD(TAG, "Start call switch activated");
  }
  
  if (end_call_switch_ && end_call_switch_->state && in_call_) {
    end_call();
  }
  
  if (accept_call_switch_ && accept_call_switch_->state && !in_call_ && ready_) {
    accept_call();
  }
  
  if (mute_switch_) {
    bool switch_state = mute_switch_->state;
    if (switch_state != muted_) {
      toggle_mute();
    }
  }
  
  update_call_state_();
  update_status_text_();
}

void IntercomComponent::generate_client_id_() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char mac_str[18];
  snprintf(mac_str, sizeof(mac_str), "%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
  client_id_ = client_id_prefix_ + std::string(mac_str).substr(0, 8);
}

void IntercomComponent::generate_session_id_() {
  char session_str[64];
  uint32_t random1 = random_uint32();
  uint32_t random2 = (uint32_t)(millis() & 0xFFFFFFFF);
  snprintf(session_str, sizeof(session_str), "%08X%08X", random1, random2);
  session_id_ = std::string(session_str);
}

#ifdef USE_ESP_IDF
void IntercomComponent::connect_websocket() {
  if (websocket_client_ != nullptr) {
    ESP_LOGW(TAG, "WebSocket already connected");
    return;
  }

  if (!wifi::global_wifi_component->is_connected()) {
    ESP_LOGW(TAG, "WiFi not connected, retrying WebSocket connection later");
    this->set_timeout(5000, [this]() { this->connect_websocket(); });
    return;
  }

  char uri[256];
  snprintf(uri, sizeof(uri), "ws://%s:%d%s", signaling_server_.c_str(), signaling_port_, signaling_path_.c_str());

  esp_websocket_client_config_t websocket_cfg = {};
  websocket_cfg.uri = uri;

  websocket_client_ = esp_websocket_client_init(&websocket_cfg);
  if (!websocket_client_) {
    ESP_LOGE(TAG, "Failed to initialize WebSocket client");
    return;
  }

  esp_websocket_register_events(websocket_client_, WEBSOCKET_EVENT_ANY, 
                                websocket_event_handler, this);
  esp_err_t ret = esp_websocket_client_start(websocket_client_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start WebSocket client: %s", esp_err_to_name(ret));
    esp_websocket_client_destroy(websocket_client_);
    websocket_client_ = nullptr;
  }
}

void IntercomComponent::disconnect_websocket() {
  if (websocket_client_) {
    esp_websocket_client_stop(websocket_client_);
    esp_websocket_client_destroy(websocket_client_);
    websocket_client_ = nullptr;
    connected_ = false;
  }
}

esp_err_t IntercomComponent::send_websocket_message(const std::string &message) {
  if (!websocket_client_ || !connected_) {
    ESP_LOGE(TAG, "WebSocket not connected");
    return ESP_ERR_INVALID_STATE;
  }
  
  return esp_websocket_client_send_text(websocket_client_, message.c_str(), 
                                        message.length(), portMAX_DELAY);
}

void IntercomComponent::websocket_event_handler(void *arg, esp_event_base_t event_base,
                                                int32_t event_id, void *event_data) {
  IntercomComponent *instance = static_cast<IntercomComponent *>(arg);
  esp_websocket_event_id_t ws_event_id = (esp_websocket_event_id_t)event_id;
  esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

  switch (ws_event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
      ESP_LOGI(TAG, "WebSocket Connected");
      instance->connected_ = true;
      instance->generate_session_id_();
      instance->room_id_ = instance->client_id_;
      instance->send_join_message_();
      break;

    case WEBSOCKET_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "WebSocket Disconnected");
      instance->connected_ = false;
      instance->in_call_ = false;
      instance->update_call_state_();
      // Reconnect after delay
      instance->set_timeout(5000, [instance]() { instance->connect_websocket(); });
      break;

    case WEBSOCKET_EVENT_DATA:
      if (data->op_code == 0x08 && data->data_len == 2) {
        ESP_LOGI(TAG, "Received closed message");
      } else {
        std::string message((char *)data->data_ptr, data->data_len);
        ESP_LOGD(TAG, "Received: %s", message.c_str());
        instance->handle_signaling_message_(message);
      }
      break;

    case WEBSOCKET_EVENT_ERROR:
      ESP_LOGE(TAG, "WebSocket error");
      break;

    default:
      break;
  }
}

#else  // Arduino framework
void IntercomComponent::connect_websocket() {
  if (!web_socket_.isConnected()) {
    web_socket_.begin(signaling_server_.c_str(), signaling_port_, signaling_path_.c_str());
  }
}

void IntercomComponent::disconnect_websocket() {
  web_socket_.disconnect();
  connected_ = false;
}

void IntercomComponent::websocket_event_(WStype_t type, uint8_t *payload, size_t length) {
  if (instance_ != nullptr) {
    instance_->handle_websocket_event_(type, payload, length);
  }
}

void IntercomComponent::handle_websocket_event_(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      ESP_LOGW(TAG, "WebSocket Disconnected");
      connected_ = false;
      in_call_ = false;
      update_call_state_();
      break;
      
    case WStype_CONNECTED:
      ESP_LOGI(TAG, "WebSocket Connected");
      connected_ = true;
      generate_session_id_();
      room_id_ = client_id_;
      send_join_message_();
      break;
      
    case WStype_TEXT:
      handle_signaling_message_(std::string((char*)payload));
      break;
      
    default:
      break;
  }
}
#endif

void IntercomComponent::send_join_message_() {
#ifdef USE_ESP_IDF
  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "type", "join");
  cJSON_AddStringToObject(json, "roomId", room_id_.c_str());
  cJSON_AddStringToObject(json, "clientId", client_id_.c_str());
  cJSON_AddStringToObject(json, "sessionId", session_id_.c_str());
  
  char *json_str = cJSON_PrintUnformatted(json);
  if (json_str) {
    send_websocket_message(json_str);
    free(json_str);
  }
  cJSON_Delete(json);
#else
  StaticJsonDocument<512> doc;
  doc["type"] = "join";
  doc["roomId"] = room_id_;
  doc["clientId"] = client_id_;
  doc["sessionId"] = session_id_;
  
  String message;
  serializeJson(doc, message);
  web_socket_.sendTXT(message);
#endif
  ESP_LOGD(TAG, "Sent join message");
}

void IntercomComponent::send_ready_message_() {
#ifdef USE_ESP_IDF
  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "type", "ready");
  cJSON_AddStringToObject(json, "roomId", room_id_.c_str());
  
  char *json_str = cJSON_PrintUnformatted(json);
  if (json_str) {
    send_websocket_message(json_str);
    free(json_str);
  }
  cJSON_Delete(json);
#else
  StaticJsonDocument<256> doc;
  doc["type"] = "ready";
  doc["roomId"] = room_id_;
  
  String message;
  serializeJson(doc, message);
  web_socket_.sendTXT(message);
#endif
}

void IntercomComponent::send_offer_message_(const std::string &sdp) {
#ifdef USE_ESP_IDF
  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "type", "offer");
  cJSON_AddStringToObject(json, "sdp", sdp.c_str());
  
  char *json_str = cJSON_PrintUnformatted(json);
  if (json_str) {
    send_websocket_message(json_str);
    free(json_str);
  }
  cJSON_Delete(json);
#else
  StaticJsonDocument<512> doc;
  doc["type"] = "offer";
  doc["sdp"] = sdp.c_str();
  
  String message;
  serializeJson(doc, message);
  web_socket_.sendTXT(message);
#endif
}

void IntercomComponent::send_answer_message_(const std::string &sdp) {
#ifdef USE_ESP_IDF
  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "type", "answer");
  cJSON_AddStringToObject(json, "sdp", sdp.c_str());
  
  char *json_str = cJSON_PrintUnformatted(json);
  if (json_str) {
    send_websocket_message(json_str);
    free(json_str);
  }
  cJSON_Delete(json);
#else
  StaticJsonDocument<512> doc;
  doc["type"] = "answer";
  doc["sdp"] = sdp.c_str();
  
  String message;
  serializeJson(doc, message);
  web_socket_.sendTXT(message);
#endif
}

void IntercomComponent::send_leave_message_() {
#ifdef USE_ESP_IDF
  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "type", "leave");
  
  char *json_str = cJSON_PrintUnformatted(json);
  if (json_str) {
    send_websocket_message(json_str);
    free(json_str);
  }
  cJSON_Delete(json);
#else
  StaticJsonDocument<128> doc;
  doc["type"] = "leave";
  
  String message;
  serializeJson(doc, message);
  web_socket_.sendTXT(message);
#endif
}

void IntercomComponent::send_candidate_message_(const std::string &candidate) {
#ifdef USE_ESP_IDF
  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "type", "candidate");
  cJSON_AddStringToObject(json, "candidate", candidate.c_str());
  
  char *json_str = cJSON_PrintUnformatted(json);
  if (json_str) {
    send_websocket_message(json_str);
    free(json_str);
  }
  cJSON_Delete(json);
#else
  StaticJsonDocument<256> doc;
  doc["type"] = "candidate";
  doc["candidate"] = candidate.c_str();
  
  String message;
  serializeJson(doc, message);
  web_socket_.sendTXT(message);
#endif
}

void IntercomComponent::handle_signaling_message_(const std::string &message) {
  ESP_LOGD(TAG, "Received signaling message: %s", message.c_str());
  
#ifdef USE_ESP_IDF
  cJSON *json = cJSON_Parse(message.c_str());
  if (!json) {
    ESP_LOGE(TAG, "Failed to parse JSON");
    return;
  }
  
  cJSON *type = cJSON_GetObjectItem(json, "type");
  if (!type || !cJSON_IsString(type)) {
    cJSON_Delete(json);
    return;
  }
  
  std::string msg_type = cJSON_GetStringValue(type);
  
  if (msg_type == "joined") {
    cJSON *role = cJSON_GetObjectItem(json, "role");
    if (role && cJSON_IsString(role)) {
      ESP_LOGI(TAG, "Joined room as: %s", cJSON_GetStringValue(role));
    }
    ready_ = true;
    send_ready_message_();
    
  } else if (msg_type == "ready") {
    ESP_LOGI(TAG, "Room is ready");
    ready_ = true;
    
  } else if (msg_type == "offer") {
    cJSON *sdp = cJSON_GetObjectItem(json, "sdp");
    if (sdp && cJSON_IsString(sdp)) {
      ESP_LOGI(TAG, "Received offer - accepting call");
      accept_call();
    }
    
  } else if (msg_type == "answer") {
    cJSON *sdp = cJSON_GetObjectItem(json, "sdp");
    if (sdp && cJSON_IsString(sdp)) {
      ESP_LOGI(TAG, "Received answer - call established");
      in_call_ = true;
      update_call_state_();
    }
    
  } else if (msg_type == "candidate") {
    cJSON *candidate = cJSON_GetObjectItem(json, "candidate");
    if (candidate && cJSON_IsString(candidate)) {
      ESP_LOGD(TAG, "Received ICE candidate: %s", cJSON_GetStringValue(candidate));
    }
    
  } else if (msg_type == "leave") {
    ESP_LOGI(TAG, "Remote left - ending call");
    end_call();
    
  } else if (msg_type == "error") {
    cJSON *error_msg = cJSON_GetObjectItem(json, "message");
    if (error_msg && cJSON_IsString(error_msg)) {
      ESP_LOGE(TAG, "Error: %s", cJSON_GetStringValue(error_msg));
    }
  }
  
  cJSON_Delete(json);
#else
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, message.c_str());
  
  if (error) {
    ESP_LOGE(TAG, "JSON parse error: %s", error.c_str());
    return;
  }
  
  std::string type = doc["type"] | "";
  
  if (type == "joined") {
    std::string role = doc["role"] | "";
    ESP_LOGI(TAG, "Joined room as: %s", role.c_str());
    ready_ = true;
    send_ready_message_();
    
  } else if (type == "ready") {
    ESP_LOGI(TAG, "Room is ready");
    ready_ = true;
    
  } else if (type == "offer") {
    std::string sdp = doc["sdp"] | "";
    ESP_LOGI(TAG, "Received offer - accepting call");
    accept_call();
    
  } else if (type == "answer") {
    std::string sdp = doc["sdp"] | "";
    ESP_LOGI(TAG, "Received answer - call established");
    in_call_ = true;
    update_call_state_();
    
  } else if (type == "candidate") {
    std::string candidate = doc["candidate"] | "";
    ESP_LOGD(TAG, "Received ICE candidate: %s", candidate.c_str());
    
  } else if (type == "leave") {
    ESP_LOGI(TAG, "Remote left - ending call");
    end_call();
    
  } else if (type == "error") {
    std::string error_msg = doc["message"] | "";
    ESP_LOGE(TAG, "Error: %s", error_msg.c_str());
  }
#endif
}

void IntercomComponent::start_call(const std::string &target_device_id) {
  if (in_call_) {
    ESP_LOGW(TAG, "Already in a call");
    return;
  }
  
  if (!connected_) {
    ESP_LOGW(TAG, "Not connected to signaling server");
    return;
  }
  
  target_device_id_ = target_device_id;
  room_id_ = target_device_id;
  generate_session_id_();
  
  send_join_message_();
  ESP_LOGI(TAG, "Initiating call to %s", target_device_id.c_str());
}

void IntercomComponent::end_call() {
  if (!in_call_) {
    return;
  }
  
  send_leave_message_();
  in_call_ = false;
  target_device_id_ = "";
  
  update_call_state_();
  ESP_LOGI(TAG, "Call ended");
}

void IntercomComponent::accept_call() {
  // Send answer (simplified - in real implementation, generate proper SDP)
  std::string local_ip = wifi::global_wifi_component->wifi_sta_ip().str();
  
  char sdp[256];
  snprintf(sdp, sizeof(sdp), "v=0\r\no=- %lu 2 IN IP4 %s\r\ns=-\r\nt=0 0\r\n",
           millis(), local_ip.c_str());
  
  send_answer_message_(sdp);
  in_call_ = true;
  
  update_call_state_();
  ESP_LOGI(TAG, "Call accepted");
}

void IntercomComponent::toggle_mute() {
  muted_ = !muted_;
  ESP_LOGI(TAG, "Mute: %s", muted_ ? "ON" : "OFF");
  
  if (mute_switch_) {
    mute_switch_->publish_state(muted_);
  }
}

void IntercomComponent::update_call_state_() {
  if (call_state_sensor_) {
    float state = 0.0f;
    if (in_call_) state = 1.0f;
    else if (connected_) state = 0.5f;
    call_state_sensor_->publish_state(state);
  }
}

void IntercomComponent::update_status_text_() {
  if (call_status_text_sensor_) {
    std::string status;
    if (in_call_) {
      status = "In Call";
      if (!target_device_id_.empty()) {
        status += " with " + target_device_id_;
      }
    } else if (connected_) {
      status = "Connected";
    } else {
      status = "Disconnected";
    }
    
    if (muted_) {
      status += " (Muted)";
    }
    
    call_status_text_sensor_->publish_state(status);
  }
  
  // Update switch states
  if (start_call_switch_) {
    // Start call switch is momentary, always off
    if (start_call_switch_->state) {
      start_call_switch_->publish_state(false);
    }
  }
  
  if (end_call_switch_) {
    // End call switch shows call state
    end_call_switch_->publish_state(in_call_);
  }
  
  if (accept_call_switch_) {
    // Accept call switch is momentary, always off
    if (accept_call_switch_->state) {
      accept_call_switch_->publish_state(false);
    }
  }
  
  if (mute_switch_) {
    mute_switch_->publish_state(muted_);
  }
}

}  // namespace intercom
}  // namespace esphome

