#include "intercom_component.h"
#include "esphome/core/log.h"

namespace esphome {
namespace intercom {

static const char *TAG = "intercom";

IntercomComponent *IntercomComponent::instance_ = nullptr;

void IntercomComponent::setup() {
  instance_ = this;
  
  ESP_LOGCONFIG(TAG, "Setting up Intercom Component...");
  
  // Generate client ID from MAC address
  generate_client_id_();
  ESP_LOGCONFIG(TAG, "Client ID: %s", client_id_.c_str());
  
  // Setup I2S
  setup_i2s_();
  
  // Setup WebSocket
  web_socket_.begin(signaling_server_.c_str(), signaling_port_, signaling_path_.c_str());
  web_socket_.onEvent(websocket_event_);
  web_socket_.setReconnectInterval(5000);
  
  // Setup UDP for audio
  udp_.begin(audio_port_);
  
  ESP_LOGCONFIG(TAG, "Intercom Component setup complete");
}

void IntercomComponent::loop() {
  web_socket_.loop();
  
  if (in_call_) {
    send_audio_packet_();
    receive_audio_packet_();
  }
}

void IntercomComponent::generate_client_id_() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char mac_str[18];
  sprintf(mac_str, "%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
  client_id_ = "esphome-" + std::string(mac_str).substr(0, 8);
}

void IntercomComponent::generate_session_id_() {
  char session_str[64];
  sprintf(session_str, "%08X%08X", (uint32_t)random(0xFFFFFFFF), (uint32_t)millis());
  session_id_ = std::string(session_str);
}

void IntercomComponent::connect_to_signaling_() {
  if (!web_socket_.isConnected()) {
    ESP_LOGD(TAG, "Connecting to signaling server...");
    web_socket_.begin(signaling_server_.c_str(), signaling_port_, signaling_path_.c_str());
  }
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

void IntercomComponent::send_join_message_() {
  StaticJsonDocument<512> doc;
  doc["type"] = "join";
  doc["roomId"] = room_id_;
  doc["clientId"] = client_id_;
  doc["sessionId"] = session_id_;
  
  String message;
  serializeJson(doc, message);
  web_socket_.sendTXT(message);
  ESP_LOGD(TAG, "Sent join: %s", message.c_str());
}

void IntercomComponent::send_ready_message_() {
  StaticJsonDocument<256> doc;
  doc["type"] = "ready";
  doc["roomId"] = room_id_;
  
  String message;
  serializeJson(doc, message);
  web_socket_.sendTXT(message);
  ESP_LOGD(TAG, "Sent ready: %s", message.c_str());
}

void IntercomComponent::send_offer_message_(const std::string &sdp) {
  StaticJsonDocument<512> doc;
  doc["type"] = "offer";
  doc["sdp"] = sdp;
  
  String message;
  serializeJson(doc, message);
  web_socket_.sendTXT(message);
  ESP_LOGD(TAG, "Sent offer");
}

void IntercomComponent::send_answer_message_(const std::string &sdp) {
  StaticJsonDocument<512> doc;
  doc["type"] = "answer";
  doc["sdp"] = sdp;
  
  String message;
  serializeJson(doc, message);
  web_socket_.sendTXT(message);
  ESP_LOGD(TAG, "Sent answer");
}

void IntercomComponent::send_leave_message_() {
  StaticJsonDocument<128> doc;
  doc["type"] = "leave";
  
  String message;
  serializeJson(doc, message);
  web_socket_.sendTXT(message);
  ESP_LOGD(TAG, "Sent leave");
}

void IntercomComponent::handle_signaling_message_(const std::string &message) {
  ESP_LOGD(TAG, "Received: %s", message.c_str());
  
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
    ready_ = true;
    ESP_LOGI(TAG, "Room is ready");
    
  } else if (type == "offer") {
    std::string sdp = doc["sdp"] | "";
    ESP_LOGI(TAG, "Received offer - accepting call");
    accept_call();
    
  } else if (type == "answer") {
    std::string sdp = doc["sdp"] | "";
    ESP_LOGI(TAG, "Received answer - call established");
    in_call_ = true;
    
  } else if (type == "candidate") {
    std::string candidate = doc["candidate"] | "";
    ESP_LOGD(TAG, "Received ICE candidate: %s", candidate.c_str());
    
  } else if (type == "leave") {
    ESP_LOGI(TAG, "Remote left - ending call");
    in_call_ = false;
    
  } else if (type == "error") {
    std::string error_msg = doc["message"] | "";
    ESP_LOGE(TAG, "Error: %s", error_msg.c_str());
  }
}

void IntercomComponent::start_call(const std::string &target_device_id) {
  if (in_call_) {
    ESP_LOGW(TAG, "Already in a call");
    return;
  }
  
  target_device_id_ = target_device_id;
  room_id_ = target_device_id;
  generate_session_id_();
  
  // Send join message for the target room
  send_join_message_();
  
  // After ready, send offer
  // (In real implementation, would generate proper SDP)
  ESP_LOGI(TAG, "Initiating call to %s", target_device_id.c_str());
}

void IntercomComponent::end_call() {
  if (!in_call_) {
    return;
  }
  
  send_leave_message_();
  in_call_ = false;
  target_device_id_ = "";
  remote_audio_port_ = 0;
  
  ESP_LOGI(TAG, "Call ended");
}

void IntercomComponent::accept_call() {
  // Send answer (simplified - in real implementation, generate proper SDP)
  std::string local_ip = WiFi.localIP().toString().c_str();
  std::string sdp = "v=0\r\no=- " + std::to_string(millis()) + 
                    " 2 IN IP4 " + local_ip + "\r\ns=-\r\nt=0 0\r\n";
  
  send_answer_message_(sdp);
  in_call_ = true;
  
  ESP_LOGI(TAG, "Call accepted");
}

void IntercomComponent::toggle_mute() {
  muted_ = !muted_;
  ESP_LOGI(TAG, "Mute: %s", muted_ ? "ON" : "OFF");
}

void IntercomComponent::setup_i2s_() {
  // Configure I2S for microphone input
  i2s_config_t i2s_mic_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = (i2s_bits_per_sample_t)BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  
  i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_BCLK,
    .ws_io_num = I2S_MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_DATA
  };
  
  i2s_driver_install(I2S_NUM_0, &i2s_mic_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);
  i2s_zero_dma_buffer(I2S_NUM_0);
  
  // Configure I2S for speaker output
  i2s_config_t i2s_spk_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = (i2s_bits_per_sample_t)BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };
  
  i2s_pin_config_t i2s_spk_pins = {
    .bck_io_num = I2S_SPK_BCLK,
    .ws_io_num = I2S_SPK_WS,
    .data_out_num = I2S_SPK_DATA,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  
  i2s_driver_install(I2S_NUM_1, &i2s_spk_config, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &i2s_spk_pins);
  
  ESP_LOGCONFIG(TAG, "I2S configured");
}

void IntercomComponent::send_audio_packet_() {
  if (muted_ || remote_audio_port_ == 0) {
    return;
  }
  
  int16_t audio_buffer[BUFFER_SIZE];
  size_t bytes_read;
  
  i2s_read(I2S_NUM_0, audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);
  
  if (bytes_read > 0) {
    // Send RTP packet (simplified - no full RTP header)
    udp_.beginPacket(remote_audio_ip_, remote_audio_port_);
    udp_.write((uint8_t*)audio_buffer, bytes_read);
    udp_.endPacket();
  }
}

void IntercomComponent::receive_audio_packet_() {
  int packet_size = udp_.parsePacket();
  if (packet_size > 0) {
    int16_t audio_buffer[BUFFER_SIZE];
    int len = udp_.read((char*)audio_buffer, sizeof(audio_buffer));
    
    if (len > 0) {
      // Play audio to speaker
      size_t bytes_written;
      i2s_write(I2S_NUM_1, audio_buffer, len, &bytes_written, portMAX_DELAY);
    }
  }
}

}  // namespace intercom
}  // namespace esphome

