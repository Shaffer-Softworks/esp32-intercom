# Integration Guide

This guide explains how to integrate ESP32 intercom devices with the Android WebRTC intercom system.

## Architecture Overview

```
┌─────────────┐         ┌──────────────┐         ┌─────────────┐
│   Android   │◄───────►│   Signaling  │◄───────►│    ESP32    │
│   Device    │ WebRTC  │    Server    │  RTP    │   Device    │
│             │         │  (Node-RED)  │         │             │
└─────────────┘         └──────────────┘         └─────────────┘
```

## Current Status

### What Works

- ✅ ESP32 ↔ Signaling Server: Full compatibility
- ✅ Same signaling protocol as Android
- ✅ Same message format
- ✅ Auto-reconnect to signaling server
- ✅ Always-on mode (stays connected)

### What Needs Work

- ⚠️ ESP32 ↔ Android: Different audio protocols
  - ESP32 uses: RTP/UDP (simplified)
  - Android uses: WebRTC (full implementation)
  - Requires modification for interoperability

## Integration Options

### Option 1: Dual Protocol Support (Recommended)

Modify the Android app to support both WebRTC and RTP protocols.

#### Implementation Steps

1. **Detect Device Type**
   ```kotlin
   fun isEsp32Device(deviceId: String): Boolean {
       return deviceId.startsWith("esp32-")
   }
   ```

2. **Add RTP Audio Handler**
   ```kotlin
   class RtpAudioHandler(private val context: Context) {
       private var udpSocket: DatagramSocket? = null
       private var audioRecord: AudioRecord? = null
       private var audioTrack: AudioTrack? = null
       
       fun start(localPort: Int, remoteIP: String, remotePort: Int) {
           // Setup UDP socket
           // Setup AudioRecord for microphone
           // Setup AudioTrack for speaker
           // Start send/receive threads
       }
   }
   ```

3. **Modify IntercomRepository**
   ```kotlin
   suspend fun connect(deviceId: String, signalingUrl: String) {
       if (isEsp32Device(deviceId)) {
           connectRtp(deviceId, signalingUrl)
       } else {
           connectWebRtc(deviceId, signalingUrl)
       }
   }
   ```

4. **Add IP/Port Exchange to Signaling**
   - Exchange IP addresses and ports via signaling messages
   - Use new message type: `audio_config`

#### Advantages

- Full interoperability
- No server changes needed
- Works on local network

#### Disadvantages

- Requires Android app modification
- More complex codebase

### Option 2: WebRTC Bridge Server

Create a bridge server that converts between WebRTC and RTP.

#### Architecture

```
Android ←→ [Bridge Server] ←→ ESP32
WebRTC      WebRTC↔RTP        RTP
```

#### Implementation

1. **WebRTC Endpoint** (for Android)
   - Receives WebRTC connections
   - Extracts audio stream

2. **RTP Endpoint** (for ESP32)
   - Sends/receives RTP packets
   - Converts to/from WebRTC audio

3. **Signaling Integration**
   - Intercepts signaling messages
   - Routes to appropriate endpoint

#### Advantages

- No Android app changes
- Centralized conversion logic
- Can add features (recording, etc.)

#### Disadvantages

- Requires server development
- Additional network hop
- More complex deployment

### Option 3: Lightweight WebRTC on ESP32

Implement a minimal WebRTC subset on ESP32.

#### Requirements

- SDP parsing/generation
- DTLS-SRTP for encryption
- ICE candidate handling
- RTP packet handling

#### Advantages

- Full WebRTC compatibility
- No Android changes
- Standard protocol

#### Disadvantages

- Very complex implementation
- High resource requirements
- May not fit on ESP32

## Recommended Approach: Option 1

### Detailed Implementation

#### 1. Create RTP Audio Handler

```kotlin
// RtpAudioHandler.kt
class RtpAudioHandler(private val context: Context) {
    private var udpSocket: DatagramSocket? = null
    private var audioRecord: AudioRecord? = null
    private var audioTrack: AudioTrack? = null
    private var isRunning = false
    private var sendJob: Job? = null
    private var receiveJob: Job? = null
    
    fun start(localPort: Int, remoteIP: String, remotePort: Int) {
        // Setup UDP
        udpSocket = DatagramSocket(localPort)
        
        // Setup audio recording
        val sampleRate = 16000
        val channelConfig = AudioFormat.CHANNEL_IN_MONO
        val audioFormat = AudioFormat.ENCODING_PCM_16BIT
        val bufferSize = AudioRecord.getMinBufferSize(sampleRate, channelConfig, audioFormat)
        
        audioRecord = AudioRecord(
            MediaRecorder.AudioSource.MIC,
            sampleRate,
            channelConfig,
            audioFormat,
            bufferSize
        )
        
        // Setup audio playback
        audioTrack = AudioTrack(
            AudioManager.STREAM_VOICE_CALL,
            sampleRate,
            AudioFormat.CHANNEL_OUT_MONO,
            audioFormat,
            bufferSize,
            AudioTrack.MODE_STREAM
        )
        
        audioRecord?.startRecording()
        audioTrack?.play()
        isRunning = true
        
        // Start send/receive threads
        startSendReceive(remoteIP, remotePort)
    }
    
    private fun startSendReceive(remoteIP: String, remotePort: Int) {
        val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
        
        sendJob = scope.launch {
            val buffer = ByteArray(1024)
            while (isRunning) {
                val bytesRead = audioRecord?.read(buffer, 0, buffer.size) ?: 0
                if (bytesRead > 0) {
                    val packet = DatagramPacket(buffer, bytesRead, InetAddress.getByName(remoteIP), remotePort)
                    udpSocket?.send(packet)
                }
                delay(10)
            }
        }
        
        receiveJob = scope.launch {
            val buffer = ByteArray(1024)
            while (isRunning) {
                val packet = DatagramPacket(buffer, buffer.size)
                try {
                    udpSocket?.receive(packet)
                    audioTrack?.write(packet.data, 0, packet.length)
                } catch (e: Exception) {
                    // Handle error
                }
            }
        }
    }
    
    fun stop() {
        isRunning = false
        sendJob?.cancel()
        receiveJob?.cancel()
        audioRecord?.stop()
        audioRecord?.release()
        audioTrack?.stop()
        audioTrack?.release()
        udpSocket?.close()
    }
}
```

#### 2. Modify Signaling Messages

Add new message type for IP/port exchange:

```kotlin
// SignalingMessage.kt
@Serializable
data class AudioConfig(
    val ip: String,
    val port: Int
) : SignalingMessage()
```

#### 3. Update IntercomRepository

```kotlin
// In IntercomRepository.kt
private var rtpAudioHandler: RtpAudioHandler? = null

suspend fun connect(deviceId: String, signalingUrl: String) {
    if (deviceId.startsWith("esp32-")) {
        connectRtp(deviceId, signalingUrl)
    } else {
        connectWebRtc(deviceId, signalingUrl)
    }
}

private suspend fun connectRtp(deviceId: String, signalingUrl: String) {
    // Initialize connection
    initializeConnection(deviceId, signalingUrl)
    
    // Setup signaling client
    setupSignalingClient(signalingUrl)
    
    // Wait for ready, then exchange IP/port
    // Start RTP audio handler
}
```

## Testing

### Test ESP32 Standalone

1. Upload code to ESP32
2. Verify WebSocket connection
3. Verify audio I/O works
4. Check serial monitor for errors

### Test Android-ESP32

1. Start call from Android to ESP32
2. Verify signaling works
3. Verify audio exchange works
4. Test bidirectional audio

### Test ESP32-Android

1. Start call from ESP32 to Android
2. Verify signaling works
3. Verify audio exchange works

## Performance Considerations

### ESP32 Limitations

- Limited RAM (typically 520KB)
- Limited processing power
- Single-core or dual-core at 240MHz

### Optimization Tips

1. **Audio Buffer Sizes**: Keep buffers small (512-1024 samples)
2. **Sample Rate**: Use 16kHz (lower than 48kHz)
3. **Bit Depth**: 16-bit is sufficient
4. **No Audio Processing**: Skip echo cancellation, noise reduction on ESP32
5. **Simple Codec**: Use PCM or simple codec (not Opus)

## Security Considerations

### Current Implementation

- No encryption (RTP is unencrypted)
- Works on local network only

### For Production

1. **DTLS-SRTP**: Implement DTLS-SRTP for encrypted RTP
2. **Authentication**: Add device authentication
3. **Network Security**: Use VPN or secure network

## Future Enhancements

- [ ] Audio codec support (Opus)
- [ ] Echo cancellation
- [ ] Noise reduction
- [ ] Volume control
- [ ] Call quality metrics
- [ ] Multi-device support

## Troubleshooting

### ESP32 Can't Connect

- Check WiFi credentials
- Verify signaling server is accessible
- Check firewall rules

### No Audio

- Verify I2S connections
- Check audio buffer sizes
- Verify UDP ports are open
- Check network connectivity

### Audio Quality Issues

- Reduce buffer sizes
- Check network latency
- Verify sample rate matches
- Check for packet loss

## References

- [WebRTC Specification](https://www.w3.org/TR/webrtc/)
- [RTP Specification](https://tools.ietf.org/html/rfc3550)
- [ESP32 I2S Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)

