/*
 * ESPHome Custom Component for Intercom
 * 
 * This component handles WebRTC signaling and audio streaming
 * Compatible with Android WebRTC Intercom System
 */

#pragma once

#include "esphome.h"
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>

namespace esphome {
namespace intercom {

class IntercomComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }
  
  // Configuration
  void set_signaling_server(const std::string &server) { signaling_server_ = server; }
  void set_signaling_port(int port) { signaling_port_ = port; }
  void set_signaling_path(const std::string &path) { signaling_path_ = path; }
  void set_audio_port(int port) { audio_port_ = port; }
  
  // Call control
  void start_call(const std::string &target_device_id);
  void end_call();
  void accept_call();
  void toggle_mute();
  
  // State
  bool is_in_call() const { return in_call_; }
  bool is_muted() const { return muted_; }
  std::string get_client_id() const { return client_id_; }

 protected:
  // Signaling
  std::string signaling_server_ = "ha.shafferco.com";
  int signaling_port_ = 1880;
  std::string signaling_path_ = "/endpoint/webrtc";
  WebSocketsClient web_socket_;
  bool connected_ = false;
  
  // Device identification
  std::string client_id_;
  std::string session_id_;
  std::string room_id_;
  std::string target_device_id_;
  
  // Call state
  bool in_call_ = false;
  bool muted_ = false;
  bool ready_ = false;
  
  // Audio
  int audio_port_ = 5004;
  WiFiUDP udp_;
  IPAddress remote_audio_ip_;
  int remote_audio_port_ = 0;
  
  // I2S Configuration
  static constexpr int SAMPLE_RATE = 16000;
  static constexpr int BITS_PER_SAMPLE = I2S_BITS_PER_SAMPLE_16BIT;
  static constexpr int BUFFER_SIZE = 1024;
  
  // I2S Pins (adjust for your hardware)
  static constexpr int I2S_MIC_BCLK = 32;
  static constexpr int I2S_MIC_WS = 25;
  static constexpr int I2S_MIC_DATA = 33;
  
  static constexpr int I2S_SPK_BCLK = 26;
  static constexpr int I2S_SPK_WS = 25;
  static constexpr int I2S_SPK_DATA = 22;
  
  // Methods
  void generate_client_id_();
  void generate_session_id_();
  void connect_to_signaling_();
  void handle_websocket_event_(WStype_t type, uint8_t *payload, size_t length);
  void handle_signaling_message_(const std::string &message);
  void send_join_message_();
  void send_ready_message_();
  void send_offer_message_(const std::string &sdp);
  void send_answer_message_(const std::string &sdp);
  void send_leave_message_();
  void setup_i2s_();
  void send_audio_packet_();
  void receive_audio_packet_();
  
  // Static callback for WebSocket
  static void websocket_event_(WStype_t type, uint8_t *payload, size_t length);
  static IntercomComponent *instance_;
};

}  // namespace intercom
}  // namespace esphome

