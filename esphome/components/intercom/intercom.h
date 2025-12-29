/*
 * ESPHome Custom Component for WebRTC Intercom
 * Compatible with Android WebRTC Intercom System
 * Supports ESP-IDF framework for ESP32-P4
 */

#pragma once

#include "esphome.h"

#ifdef USE_ESP_IDF
#include "esp_websocket_client.h"
#include "cJSON.h"
#include "esp_peer.h"
#include "esp_webrtc.h"
#else
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#endif

#include <driver/i2s.h>

namespace esphome {
namespace intercom {

class IntercomComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI - 1.0f; }
  void dump_config() override;
  
  // Configuration
  void set_signaling_server(const std::string &server) { signaling_server_ = server; }
  void set_signaling_port(int port) { signaling_port_ = port; }
  void set_signaling_path(const std::string &path) { signaling_path_ = path; }
  void set_client_id_prefix(const std::string &prefix) { client_id_prefix_ = prefix; }
  
  // Home Assistant entities
  void set_call_state_sensor(sensor::Sensor *sensor) { call_state_sensor_ = sensor; }
  void set_call_status_text_sensor(text_sensor::TextSensor *sensor) { call_status_text_sensor_ = sensor; }
  void set_target_device_text_sensor(text_sensor::TextSensor *sensor) { target_device_text_sensor_ = sensor; }
  void set_start_call_switch(switch_::Switch *sw) { start_call_switch_ = sw; }
  void set_end_call_switch(switch_::Switch *sw) { end_call_switch_ = sw; }
  void set_accept_call_switch(switch_::Switch *sw) { accept_call_switch_ = sw; }
  void set_mute_switch(switch_::Switch *sw) { mute_switch_ = sw; }
  
  // Actions
  void start_call(const std::string &target_device_id);
  void end_call();
  void accept_call();
  void toggle_mute();
  
  // Configuration
  void set_auto_accept(bool auto_accept) { auto_accept_ = auto_accept; }
  void set_auto_connect(bool auto_connect) { auto_connect_ = auto_connect; }
  
  // State
  bool is_in_call() const { return in_call_; }
  bool is_muted() const { return muted_; }
  bool is_connected() const { return connected_; }
  std::string get_client_id() const { return client_id_; }
  std::string get_current_target() const { return target_device_id_; }

 protected:
  // Signaling
  std::string signaling_server_ = "ha.shafferco.com";
  int signaling_port_ = 1880;
  std::string signaling_path_ = "/endpoint/webrtc";
  std::string client_id_prefix_ = "esphome-";
  
  bool connected_ = false;
  bool in_call_ = false;
  bool muted_ = false;
  bool ready_ = false;
  bool auto_accept_ = true;      // Automatically accept incoming calls
  bool auto_connect_ = true;     // Automatically send offer when ready
  bool waiting_for_ready_ = false; // Waiting for ready to send offer
  
  // Device identification
  std::string client_id_;
  std::string session_id_;
  std::string room_id_;
  std::string target_device_id_;
  
#ifdef USE_ESP_IDF
  esp_websocket_client_handle_t websocket_client_ = nullptr;
  esp_peer_handle_t webrtc_peer_ = nullptr;  // WebRTC peer connection
  bool is_offerer_ = false;  // true when initiating call, false when receiving
  
  void websocket_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
  void connect_websocket();
  void disconnect_websocket();
  esp_err_t send_websocket_message(const std::string &message);
  
  // WebRTC methods
  esp_err_t init_webrtc_peer(bool is_offerer);
  void deinit_webrtc_peer();
  esp_err_t create_offer();
  esp_err_t create_answer();
  esp_err_t set_remote_description(const std::string &sdp, bool is_offer);
  esp_err_t add_ice_candidate(const std::string &candidate);
  void on_ice_candidate(const char *candidate);
  void on_peer_connection_state(esp_peer_connection_state_t state);
  
  // Audio callbacks for WebRTC
  static int audio_capture_cb(void *ctx, void *buffer, int len);
  static int audio_render_cb(void *ctx, void *buffer, int len);
  static void ice_candidate_cb(void *ctx, const char *candidate);
  static void peer_connection_state_cb(void *ctx, esp_peer_connection_state_t state);
#else
  WebSocketsClient web_socket_;
  static void websocket_event_(WStype_t type, uint8_t *payload, size_t length);
  static IntercomComponent *instance_;
#endif
  
  // Signaling methods
  void generate_client_id_();
  void generate_session_id_();
  void handle_signaling_message_(const std::string &message);
  void send_join_message_();
  void send_ready_message_();
  void send_offer_message_(const std::string &sdp);
  void send_answer_message_(const std::string &sdp);
  void send_leave_message_();
  void send_candidate_message_(const std::string &candidate);
  
  // State update
  void update_call_state_();
  void update_status_text_();
  
  // Methods
  void send_offer_for_call_();
  
  // Home Assistant entities
  sensor::Sensor *call_state_sensor_{nullptr};
  text_sensor::TextSensor *call_status_text_sensor_{nullptr};
  text_sensor::TextSensor *target_device_text_sensor_{nullptr};
  switch_::Switch *start_call_switch_{nullptr};
  switch_::Switch *end_call_switch_{nullptr};
  switch_::Switch *accept_call_switch_{nullptr};
  switch_::Switch *mute_switch_{nullptr};
};

}  // namespace intercom
}  // namespace esphome

