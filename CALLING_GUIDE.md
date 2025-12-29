# Calling and Auto-Connect Guide

Complete guide for making and receiving calls with the ESPHome intercom component.

## Features

- **Auto-Connect**: When you initiate a call, the device automatically sends an offer when the room is ready
- **Auto-Accept**: Incoming calls are automatically accepted (configurable)
- **Target Device Selection**: Specify which device to call via Home Assistant

## Configuration

### Enable Auto-Connect Features

In your ESPHome configuration:

```yaml
intercom:
  auto_accept: true    # Automatically accept incoming calls
  auto_connect: true   # Automatically send offer when room is ready
```

### Setting Up Target Device Selection

1. **Input Text Field** (already in `intercom_waveshare.yaml`):
   ```yaml
   input_text:
     - id: intercom_target_device_input
       name: "Intercom Target Device ID"
   ```

2. **Use in Home Assistant**: The input text field appears as `input_text.intercom_target_device_id`

## How to Call Another Device

### Method 1: Via Home Assistant UI

1. **Set Target Device**:
   - Open Home Assistant
   - Find `input_text.intercom_target_device_id`
   - Enter the device ID (e.g., `waveshare-12345678` or `esp32-ABCDEF12`)
   - Click "Update"

2. **Start Call**:
   - Find `switch.start_intercom_call`
   - Turn it on
   - The device will automatically:
     - Join the target device's room
     - Wait for room to be ready (when both devices are in the room)
     - Send an offer automatically (if `auto_connect: true`)
     - Establish the call

### Method 2: Via Home Assistant Automation

```yaml
automation:
  - alias: "Call Doorbell"
    trigger:
      - platform: state
        entity_id: binary_sensor.doorbell_button
        to: 'on'
    action:
      - service: input_text.set_value
        target:
          entity_id: input_text.intercom_target_device_id
        data:
          value: "doorbell-12345678"
      - service: switch.turn_on
        target:
          entity_id: switch.start_intercom_call
```

### Method 3: Via ESPHome Lambda

```yaml
script:
  - id: call_specific_device
    then:
      - lambda: |-
          id(intercom_device).start_call("target-device-id");
```

## Auto-Connect Flow

### When Initiating a Call:

1. **User Action**: Start call to target device
2. **Join Room**: Device joins target device's room
3. **Wait for Ready**: Signaling server sends "ready" when 2 peers are in room
4. **Auto-Send Offer**: Device automatically sends WebRTC offer
5. **Receive Answer**: Target device responds with answer
6. **Call Established**: Audio connection is established

### When Receiving a Call:

1. **Receive Offer**: Incoming offer from another device
2. **Auto-Accept** (if enabled): Automatically send answer
3. **Call Established**: Audio connection is established

## Device ID Format

Device IDs are typically generated from MAC addresses:
- ESPHome devices: `esphome-XXXXXXXX` or `waveshare-XXXXXXXX`
- ESP32 devices: `esp32-XXXXXXXX`
- Android devices: `station-XXXXXXXX` or `handset-XXXXXXXX`

You can find device IDs:
- In Home Assistant: Check the `text_sensor.intercom_status` entity
- In logs: Device ID is logged on startup
- On signaling server: Check active clients

## Call States

Monitor call state via `sensor.intercom_call_state`:
- **0.0**: Disconnected from signaling server
- **0.5**: Connected, waiting/ready
- **1.0**: In active call

Or use `text_sensor.intercom_status` for human-readable status:
- "Disconnected"
- "Connected"
- "In Call with [device-id]"

## Ending Calls

### Manual End:
- Use `switch.end_intercom_call` in Home Assistant
- Or call `id(intercom_device).end_call()` in ESPHome lambda

### Automatic End:
- Other device leaves the call
- Network disconnection
- Signaling server disconnects

## Troubleshooting

### Call Not Connecting

1. **Check Target Device ID**: Ensure it's correct and the device is online
2. **Check Signaling Server**: Both devices must be connected to the same server
3. **Check Logs**: Enable debug logging:
   ```yaml
   logger:
     logs:
       intercom: debug
   ```
4. **Check Room State**: Verify signaling server shows both devices in the room

### Auto-Connect Not Working

1. Verify `auto_connect: true` in configuration
2. Check that "ready" message is received (check logs)
3. Verify both devices are in the same room
4. Check that offer is being sent (check logs)

### Auto-Accept Not Working

1. Verify `auto_accept: true` in configuration
2. Check that offer messages are being received
3. Check logs for errors during answer generation

## Example Home Assistant Dashboard

Create a simple dashboard:

```yaml
type: entities
entities:
  - entity: text_sensor.intercom_status
    name: Status
  - entity: sensor.intercom_call_state
    name: Call State
  - entity: input_text.intercom_target_device_id
    name: Target Device
  - entity: switch.start_intercom_call
    name: Start Call
  - entity: switch.end_intercom_call
    name: End Call
  - entity: switch.intercom_mute
    name: Mute
```

## Advanced: Multiple Target Devices

You can create buttons for common targets:

```yaml
input_button:
  - id: call_doorbell
    name: "Call Doorbell"
    
automation:
  - alias: "Call Doorbell Button"
    trigger:
      - platform: state
        entity_id: input_button.call_doorbell
    action:
      - service: input_text.set_value
        target:
          entity_id: input_text.intercom_target_device_id
        data:
          value: "doorbell-12345678"
      - service: switch.turn_on
        target:
          entity_id: switch.start_intercom_call
```

## Next Steps

- Integrate with doorbell buttons
- Add voice prompts for call status
- Create a call history log
- Add call quality monitoring

