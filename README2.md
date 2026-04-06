# RobStride Motor Protocol Switch Guide

## Problem: Motor Not Communicating via CAN

If your RobStride 03 motor is not communicating over CAN (works with RobStride 06 but not 03), the motor has likely been switched to a different communication protocol (e.g., RS485, USB, or other modes).

## Solution: Switch Back to CAN Protocol

Use the `SET_PROTOCOL` command to restore CAN communication.

### CAN ID Format

```
ext_id = (SET_PROTOCOL << 24) | (HOST_ID << 8) | motor_id
```

Where:
- `SET_PROTOCOL` = 25 (decimal)
- `HOST_ID` = 0xFF
- `motor_id` = your motor's CAN ID (e.g., 1, 127)

### Data Format

```
data[0] = protocol_mode (uint8_t)
```

**Protocol Mode Values:**
| Value | Protocol |
|-------|----------|
| 0 | CAN (default) |
| 1 | RS485 |
| 2 | USB |
| 3 | RS232 |

## Methods to Switch Back to CAN

### Method 1: Using cansend (Quick Command)

```bash
# Syntax: cansend <interface> <can_id>#<data>
# Example for motor ID 1:
cansend can0 "25#00"     # Switch motor 1 to CAN protocol
cansend can0 "25#FF01"   # Using extended ID format with host_id 0xFF
```

### Method 2: Using Python Script

Create a script `switch_to_can.py`:

```python
#!/usr/bin/env python3
import can
import struct

MOTOR_ID = 1  # Change to your motor ID
HOST_ID = 0xFF
SET_PROTOCOL = 25
CAN_INTERFACE = "can0"

def switch_to_can_protocol():
    """Switch motor back to CAN protocol mode."""
    bus = can.interface.Bus(channel=CAN_INTERFACE, bustype='socketcan')

    # Build extended CAN ID
    can_id = (SET_PROTOCOL << 24) | (HOST_ID << 8) | MOTOR_ID

    # Protocol mode: 0 = CAN
    msg = can.Message(
        arbitration_id=can_id,
        data=[0],  # 0 = CAN protocol
        is_extended_id=True
    )

    try:
        bus.send(msg)
        print(f"Switched motor {MOTOR_ID} to CAN protocol successfully")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        bus.shutdown()

if __name__ == "__main__":
    switch_to_can_protocol()
```

Run with:
```bash
python3 switch_to_can.py
```

### Method 3: Using SocketCAN with C++

Add this function to your code:

```cpp
bool set_can_protocol(int sock, int motor_id, uint8_t protocol_mode) {
    // SET_PROTOCOL = 25
    uint32_t ext_id = (25 << 24) | (0xFF << 8) | motor_id;
    uint8_t data[8] = {0};
    data[0] = protocol_mode;  // 0 = CAN

    return send_frame(sock, ext_id, data, 8);
}
```

## Verification Steps

1. After sending the protocol switch command, verify with `candump`:
   ```bash
   candump can0
   ```

2. You should see status frames coming back from the motor

3. If still no response, try:
   - Power cycling the motor
   - Checking CAN wiring
   - Verifying motor ID matches

## Alternative: Reset to Factory Defaults

If switching protocols doesn't work, you may need to reset the motor parameters:

```bash
# Using candump to listen first
candump can0

# Try enabling first (sometimes helps)
cansend can0 "03#FF01"  # ENABLE command for motor 1
```

## Troubleshooting

| Symptom | Possible Cause | Solution |
|---------|----------------|----------|
| No CAN frames received | Motor in RS485/USB mode | Send SET_PROTOCOL with mode 0 |
| Intermittent responses | Loose wiring | Check CAN_H/CAN_L connections |
| Motor ID mismatch | Wrong motor ID configured | Verify motor ID with candump |
| Still not working after protocol switch | Motor needs power cycle | Turn motor off/on |

## Notes

- RobStride 06 may have different default protocol settings than RobStride 03
- Some motors automatically detect CAN mode on startup, but others retain their last protocol setting
- If the motor was previously used with RS485, it will stay in RS485 mode until explicitly switched back
