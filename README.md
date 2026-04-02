# RobStride Motor Control Node

ROS2 node for controlling RobStride motors via CAN bus using the MIT control mode.

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    RobStride Control Node                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Subscribers:                  Publishers:                        │
│  ┌──────────────────┐        ┌──────────────────────────┐      │
│  │ target_position   │        │ motor_state (JointState) │      │
│  │ target_kp         │        │ motor_status (Int32)     │      │
│  │ target_kd         │        └──────────────────────────┘      │
│  └──────────────────┘                                            │
│           │                                                      │
│           ▼                                                      │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                   Control Loop (50 Hz)                    │   │
│  │  1. Send MIT mode command frame                          │   │
│  │  2. Read motor status feedback                           │   │
│  │  3. Publish joint state                                  │   │
│  └──────────────────────────────────────────────────────────┘   │
│           │                                                      │
│           ▼                                                      │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                     CanInterface                          │   │
│  │              (SocketCAN wrapper)                          │   │
│  └──────────────────────────────────────────────────────────┘   │
│           │                                                      │
└───────────┼──────────────────────────────────────────────────────┘
            │
            ▼ CAN (can0)
    ┌───────────────────┐
    │   RobStride Motor │
    │      (RS-03)      │
    └───────────────────┘
```

## Topics

### Subscriptions

Ex_command. 
ros2 topic pub /target_position std_msgs/msg/Float64 "{data: 0.5}" --once 
ros2 topic pub /target_kp std_msgs/msg/Float64 "{data: 0.5}" --once 
ros2 topic pub /target_kd std_msgs/msg/Float64 "{data: 0.5}" --once 

| Topic | Type | Description |
|-------|------|-------------|
| `/target_position` | `std_msgs/Float64` | Target position in radians (±4π) |
| `/target_kp` | `std_msgs/Float64` | Proportional gain (0-5000 Nm/rad) |
| `/target_kd` | `std_msgs/Float64` | Derivative gain (0-100 Nm·s/rad) |

### Publications

Ex_command. ros2 topic echo /motor_state

| Topic | Type | Description |
|-------|------|-------------|
| `/motor_state` | `sensor_msgs/JointState` | Current position, velocity, torque |
| `/motor_status` | `std_msgs/Int32` | Connection status (1=connected) |

## Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `motor_id` | int | 1 | Motor CAN ID (1-255) |
| `can_interface` | string | "can0" | CAN interface name |
| `control_frequency` | double | 50.0 | Control loop frequency (Hz) |
| `default_kp` | double | 100.0 | Default proportional gain |
| `default_kd` | double | 2.0 | Default derivative gain |

## MIT Mode Protocol

The node uses MIT mode (Mode 0) for motor control with the following frame format:

**Command Frame (TX)**
- CAN ID: `(OPERATION_CONTROL << 24) | (torque_ff << 8) | motor_id`
- Data: `[pos_hi, pos_lo, vel_hi, vel_lo, kp_hi, kp_lo, kd_hi, kd_lo]`
- All 16-bit values are big-endian and scaled from physical units

**Status Frame (RX)**
- CAN ID: `(OPERATION_STATUS << 24) | ...`
- Data: `[pos, vel, torque]` as scaled 16-bit values

### Scaling Factors (RS-03)

| Parameter | Scale | Range |
|-----------|-------|-------|
| Position | 4π rad | ±12.57 rad |
| Velocity | 50 rad/s | ±50 rad/s |
| Torque | 60 Nm | ±60 Nm |
| KP | 5000 | 0-5000 Nm/rad |
| KD | 100 | 0-100 Nm·s/rad |

## Build & Run

### Build

```bash
cd ~/ros2_ws
colcon build --packages-select rs_control
source install/setup.bash
```

### Run Control Node

```bash
# Basic usage
ros2 run rs_control main_control_node

# With custom parameters
ros2 run rs_control main_control_node --ros-args \
  -p motor_id:=127 \
  -p can_interface:=can0 \
  -p control_frequency:=50.0
```

### Run State Reader Node (Read-Only)

The state reader node only reads and publishes motor state without sending control commands.

```bash
# Basic usage
ros2 run rs_control motor_state_reader_node

# With custom parameters
ros2 run rs_control motor_state_reader_node --ros-args \
  -p motor_id:=127 \
  -p can_interface:=can0 \
  -p read_frequency:=50.0
```

### Launch Files

```bash
# Using launch file
ros2 launch rs_control motor_state_reader.launch.py
```

### Prerequisites

1. **CAN Interface Setup**
   ```bash
   # Bring up CAN interface
   sudo ip link set can0 up type can bitrate 1000000
   sudo ip link set up can0

   # Verify interface is up
   ip -details link show can0
   ```

2. **Verify Motor Communication**
   ```bash
   # Listen for CAN frames
   candump can0
   ```

## Control Modes

The motor supports multiple control modes (configured via protocol):

| Mode | Value | Description |
|------|-------|-------------|
| MIT_MODE | 0 | Position + KP/KD control |
| POSITION_MODE | 1 | Pure position control |
| SPEED_MODE | 2 | Velocity control |
| TORQUE_MODE | 3 | Torque control |

Currently, the node operates in MIT_MODE.

## Troubleshooting

### Motor not responding

1. Check CAN interface is up: `ip link show can0`
2. Verify motor ID matches: `candump can0 | grep <motor_id>`
3. Check if motor is enabled (can be done via RobStride studio)

### Build errors

```bash
# Clean and rebuild
rm -rf build/ install/ log/
colcon build --packages-select rs_control
```

### Permission denied (CAN)

```bash
# Run with sudo or add user to can group
sudo usermod -aG can $USER
# Log out and back in
```

## File Structure

```
rs_control/
├── include/rs_control/
│   ├── can_interface.h    # CAN interface class declaration
│   ├── protocol.h         # RobStride protocol constants
│   └── rs_lib.h           # Main library header
├── src/
│   ├── can_interface.cpp      # CAN interface implementation
│   ├── main_control_node.cpp  # ROS2 control node (full control)
│   ├── motor_state_reader_node.cpp  # ROS2 read-only state node
│   └── rs_lib.cpp             # Protocol functions (legacy)
├── launch/
│   └── motor_state_reader.launch.py  # Launch file for state reader
├── CMakeLists.txt
├── package.xml
└── README.md
```
