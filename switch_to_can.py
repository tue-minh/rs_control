import can
import struct

MOTOR_ID = 127  # Change to your motor ID
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
        data=[1],  # 0 = CAN protocol
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