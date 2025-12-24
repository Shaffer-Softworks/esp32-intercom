/*
 * ESP32 Intercom Client
 * Compatible with Android WebRTC Intercom System
 * 
 * This implementation uses:
 * - WebSocket for signaling (same protocol as Android)
 * - RTP/UDP for audio streaming (simpler than full WebRTC)
 * 
 * Hardware Requirements:
 * - ESP32 development board
 * - I2S microphone (e.g., INMP441)
 * - I2S amplifier/speaker (e.g., MAX98357A)
 * 
 * Libraries Required:
 * - WebSockets by Markus Sattler
 * - ArduinoJson by Benoit Blanchon
 * - WiFi (included with ESP32)
 */

#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>

// ============================================================================
// CONFIGURATION - Modify these for your setup
// ============================================================================

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Signaling Server Configuration
const char* signalingServer = "ha.shafferco.com";
const int signalingPort = 1880;
const char* signalingPath = "/endpoint/webrtc";

// Audio Configuration
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_32
#define I2S_MIC_LEFT_RIGHT GPIO_NUM_25
#define I2S_MIC_SERIAL_DATA GPIO_NUM_33

#define I2S_SPEAKER_SERIAL_CLOCK GPIO_NUM_26
#define I2S_SPEAKER_LEFT_RIGHT GPIO_NUM_25
#define I2S_SPEAKER_SERIAL_DATA GPIO_NUM_22

#define SAMPLE_RATE 16000
#define BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
#define I2S_CHANNELS I2S_CHANNEL_MONO
#define BUFFER_SIZE 1024

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// WebSocket client
WebSocketsClient webSocket;

// Device identification
String clientId;
String sessionId;
String roomId;
String targetDeviceId;

// Connection state
bool isConnected = false;
bool isInCall = false;
bool isReady = false;
bool muted = false;

// Audio streaming
WiFiUDP udp;
IPAddress remoteAudioIP;
int remoteAudioPort = 0;
int localAudioPort = 5004;

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

void generateClientId();
void generateSessionId();
void connectToSignaling();
void handleWebSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void handleSignalingMessage(String message);
void sendSignalingMessage(String type, JsonObject& data);
void startAudio();
void stopAudio();
void sendAudioPacket();
void receiveAudioPacket();
void startCall(String deviceId);
void endCall();
void acceptCall();
void toggleMute();

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("ESP32 Intercom Client");
  Serial.println("========================================\n");
  
  // Generate device ID
  generateClientId();
  Serial.printf("Client ID: %s\n", clientId.c_str());
  
  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connection failed!");
    Serial.println("Please check your credentials and try again.");
    return;
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signal strength (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  
  // Setup WebSocket
  webSocket.begin(signalingServer, signalingPort, signalingPath);
  webSocket.onEvent(handleWebSocketEvent);
  webSocket.setReconnectInterval(5000);
  
  // Setup UDP for audio
  udp.begin(localAudioPort);
  Serial.printf("UDP audio port: %d\n", localAudioPort);
  
  // Initialize I2S for audio
  startAudio();
  
  Serial.println("\nESP32 Intercom Ready!");
  Serial.println("Waiting for connection to signaling server...\n");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  webSocket.loop();
  
  if (isInCall && !muted) {
    sendAudioPacket();
  }
  
  if (isInCall) {
    receiveAudioPacket();
  }
  
  delay(10);
}

// ============================================================================
// DEVICE ID GENERATION
// ============================================================================

void generateClientId() {
  // Use MAC address to generate unique client ID
  uint64_t chipid = ESP.getEfuseMac();
  // Extract MAC address bytes (6 bytes total)
  uint8_t mac[6];
  mac[0] = (chipid >> 40) & 0xFF;
  mac[1] = (chipid >> 32) & 0xFF;
  mac[2] = (chipid >> 24) & 0xFF;
  mac[3] = (chipid >> 16) & 0xFF;
  mac[4] = (chipid >> 8) & 0xFF;
  mac[5] = chipid & 0xFF;
  char macStr[18];
  sprintf(macStr, "%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
  clientId = "esp32-" + String(macStr).substring(0, 8);
}

void generateSessionId() {
  // Generate a simple session ID
  char sessionStr[64];
  sprintf(sessionStr, "%08X%08X", (uint32_t)random(0xFFFFFFFF), (uint32_t)millis());
  sessionId = String(sessionStr);
}

// ============================================================================
// SIGNALING - WebSocket
// ============================================================================

void connectToSignaling() {
  if (!webSocket.isConnected()) {
    Serial.println("Connecting to signaling server...");
    webSocket.begin(signalingServer, signalingPort, signalingPath);
  }
}

void handleWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WebSocket] Disconnected");
      isConnected = false;
      isInCall = false;
      break;
      
    case WStype_CONNECTED:
      Serial.println("[WebSocket] Connected to signaling server");
      isConnected = true;
      // Join room with our client ID (for always-on mode)
      generateSessionId();
      roomId = clientId;
      sendJoinMessage();
      break;
      
    case WStype_TEXT:
      handleSignalingMessage(String((char*)payload));
      break;
      
    case WStype_ERROR:
      Serial.printf("[WebSocket] Error: %s\n", payload);
      break;
      
    default:
      break;
  }
}

void sendJoinMessage() {
  StaticJsonDocument<512> doc;
  doc["type"] = "join";
  doc["roomId"] = roomId;
  doc["clientId"] = clientId;
  doc["sessionId"] = sessionId;
  
  String message;
  serializeJson(doc, message);
  webSocket.sendTXT(message);
  Serial.printf("[Signaling] Sent join: %s\n", message.c_str());
}

void sendReadyMessage() {
  StaticJsonDocument<256> doc;
  doc["type"] = "ready";
  doc["roomId"] = roomId;
  
  String message;
  serializeJson(doc, message);
  webSocket.sendTXT(message);
  Serial.println("[Signaling] Sent ready");
}

void sendLeaveMessage() {
  StaticJsonDocument<128> doc;
  doc["type"] = "leave";
  
  String message;
  serializeJson(doc, message);
  webSocket.sendTXT(message);
  Serial.println("[Signaling] Sent leave");
}

void handleSignalingMessage(String message) {
  Serial.printf("[Signaling] Received: %s\n", message.c_str());
  
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.printf("[Signaling] JSON parse error: %s\n", error.c_str());
    return;
  }
  
  String type = doc["type"] | "";
  
  if (type == "joined") {
    String role = doc["role"] | "";
    Serial.printf("[Signaling] Joined room as: %s\n", role.c_str());
    isReady = true;
    sendReadyMessage();
    
  } else if (type == "ready") {
    isReady = true;
    Serial.println("[Signaling] Room is ready");
    
  } else if (type == "offer") {
    // Incoming call - we need to handle this
    String sdp = doc["sdp"] | "";
    Serial.println("[Signaling] Received offer - accepting call");
    acceptCall();
    
  } else if (type == "answer") {
    // Call answered
    String sdp = doc["sdp"] | "";
    Serial.println("[Signaling] Received answer - call established");
    isInCall = true;
    
  } else if (type == "candidate") {
    // ICE candidate - for simplified version, we might skip this
    // or use it to exchange IP/port information
    String candidate = doc["candidate"] | "";
    Serial.printf("[Signaling] Received ICE candidate: %s\n", candidate.c_str());
    
  } else if (type == "leave") {
    Serial.println("[Signaling] Remote left - ending call");
    endCall();
    
  } else if (type == "error") {
    String errorMsg = doc["message"] | "";
    Serial.printf("[Signaling] Error: %s\n", errorMsg.c_str());
    
  } else if (type == "replaced") {
    String bySessionId = doc["bySessionId"] | "";
    Serial.printf("[Signaling] Session replaced by: %s\n", bySessionId.c_str());
    // Reconnect if needed
  }
}

// ============================================================================
// CALL MANAGEMENT
// ============================================================================

void startCall(String deviceId) {
  if (isInCall) {
    Serial.println("[Call] Already in a call");
    return;
  }
  
  targetDeviceId = deviceId;
  roomId = deviceId;
  generateSessionId();
  
  // Send join message for the target room
  sendJoinMessage();
  
  Serial.printf("[Call] Initiating call to %s\n", deviceId.c_str());
}

void endCall() {
  if (!isInCall) {
    return;
  }
  
  sendLeaveMessage();
  isInCall = false;
  targetDeviceId = "";
  remoteAudioPort = 0;
  
  Serial.println("[Call] Call ended");
}

void acceptCall() {
  // Send answer (simplified - in real implementation, generate proper SDP)
  StaticJsonDocument<512> doc;
  doc["type"] = "answer";
  doc["sdp"] = "v=0\r\no=- " + String(millis()) + " 2 IN IP4 " + WiFi.localIP().toString() + "\r\ns=-\r\nt=0 0\r\n";
  
  String message;
  serializeJson(doc, message);
  webSocket.sendTXT(message);
  
  isInCall = true;
  Serial.println("[Call] Call accepted");
}

void toggleMute() {
  muted = !muted;
  Serial.printf("[Call] Mute: %s\n", muted ? "ON" : "OFF");
}

// ============================================================================
// AUDIO - I2S
// ============================================================================

void startAudio() {
  Serial.println("[Audio] Initializing I2S...");
  
  // Configure I2S for microphone input
  i2s_config_t i2s_mic_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = BITS_PER_SAMPLE,
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
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA
  };
  
  i2s_driver_install(I2S_NUM_0, &i2s_mic_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);
  i2s_zero_dma_buffer(I2S_NUM_0);
  
  // Configure I2S for speaker output
  i2s_config_t i2s_spk_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = BITS_PER_SAMPLE,
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
    .bck_io_num = I2S_SPEAKER_SERIAL_CLOCK,
    .ws_io_num = I2S_SPEAKER_LEFT_RIGHT,
    .data_out_num = I2S_SPEAKER_SERIAL_DATA,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  
  i2s_driver_install(I2S_NUM_1, &i2s_spk_config, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &i2s_spk_pins);
  
  Serial.println("[Audio] I2S initialized");
}

void stopAudio() {
  i2s_stop(I2S_NUM_0);
  i2s_stop(I2S_NUM_1);
  Serial.println("[Audio] I2S stopped");
}

void sendAudioPacket() {
  if (remoteAudioPort == 0) {
    return;
  }
  
  // Read audio from microphone
  int16_t audioBuffer[BUFFER_SIZE];
  size_t bytesRead;
  
  i2s_read(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytesRead, portMAX_DELAY);
  
  if (bytesRead > 0) {
    // Send RTP packet (simplified - no full RTP header)
    udp.beginPacket(remoteAudioIP, remoteAudioPort);
    udp.write((uint8_t*)audioBuffer, bytesRead);
    udp.endPacket();
  }
}

void receiveAudioPacket() {
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    int16_t audioBuffer[BUFFER_SIZE];
    int len = udp.read((char*)audioBuffer, sizeof(audioBuffer));
    
    if (len > 0) {
      // Store remote IP/port for sending
      remoteAudioIP = udp.remoteIP();
      remoteAudioPort = udp.remotePort();
      
      // Play audio to speaker
      size_t bytesWritten;
      i2s_write(I2S_NUM_1, audioBuffer, len, &bytesWritten, portMAX_DELAY);
    }
  }
}

