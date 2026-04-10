# RobStride Motor Control Node

ROS2 node for controlling RobStride motors via CAN bus using the MIT control mode.

## Features

- **Manual Position Control**: Set target positions via ROS topics
- **Sine Wave Motion**: Generate smooth sine wave trajectories for testing
- **Dual Motor Support**: Independent control of two motors (RS-03 and RS-06)
- **Real-time Feedback**: Publish motor states and status

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                 Dual Motor Control Node                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Subscribers:                  Publishers:                        │
│  ┌──────────────────┐        ┌──────────────────────────┐      │
│  │ target_position_* │        │ motor_*_state (JointState)│      │
│  │ sine_wave_*       │        │ motor_status (Int32)     │      │
│  │ target_kp/kd      │        └──────────────────────────┘      │
│  └──────────────────┘                                            │
│           │                                                      │
│           ▼                                                      │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                   Control Loop (50 Hz)                    │   │
│  │  1. Generate position commands (manual or sine wave)     │   │
│  │  2. Send MIT mode command frames                          │   │
│  │  3. Read motor status feedback                            │   │
│  │  4. Publish joint states                                  │   │
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
    ┌───────────────────┐    ┌───────────────────┐
    │   RobStride RS-03 │    │   RobStride RS-06 │
    │     (ID: 1)       │    │     (ID: 127)     │
    └───────────────────┘    └───────────────────┘
```

## Topics

### Subscriptions

#### Manual Position Control
- `target_position_rs03` (std_msgs/Float64): Target position for RS-03 motor
- `target_position_rs06` (std_msgs/Float64): Target position for RS-06 motor
- `target_kp` (std_msgs/Float64): Position gain (shared)
- `target_kd` (std_msgs/Float64): Velocity gain (shared)

#### Sine Wave Control
- `sine_wave_rs03` (rs_control/SineWaveParameters): Sine wave parameters for RS-03
- `sine_wave_rs06` (rs_control/SineWaveParameters): Sine wave parameters for RS-06

### Publishers
- `motor_rs03_state` (sensor_msgs/JointState): RS-03 motor feedback
- `motor_rs06_state` (sensor_msgs/JointState): RS-06 motor feedback
- `motor_status` (std_msgs/Int32): Combined motor status (bit 0: RS-03, bit 1: RS-06)

## Usage Examples

### Manual Position Control
```bash
# Set RS-03 position
ros2 topic pub /target_position_rs03 std_msgs/msg/Float64 "{data: 0.5}" --once

# Set RS-06 position
ros2 topic pub /target_position_rs06 std_msgs/msg/Float64 "{data: 1.0}" --once

# Set gains
ros2 topic pub /target_kp std_msgs/msg/Float64 "{data: 2.0}" --once
ros2 topic pub /target_kd std_msgs/msg/Float64 "{data: 0.1}" --once
```

### Sine Wave Control
```bash
# Start sine wave motion for RS-03
ros2 topic pub /sine_wave_rs03 rs_control/msg/SineWaveParameters "{
  amplitude: 1.0,
  frequency: 0.5,
  velocity_limit: 10.0,
  kp: 2.0,
  kd: 0.1
}" --once

# Start sine wave motion for RS-06 with different parameters
ros2 topic pub /sine_wave_rs06 rs_control/msg/SineWaveParameters "{
  amplitude: 0.8,
  frequency: 0.3,
  velocity_limit: 8.0,
  kp: 1.5,
  kd: 0.05
}" --once
```

### Monitor Motor State
```bash
# Monitor RS-03 state
ros2 topic echo /motor_rs03_state

# Monitor RS-06 state
ros2 topic echo /motor_rs06_state

# Monitor status
ros2 topic echo /motor_status
```

## Testing

A test script is provided to demonstrate the sine wave functionality:

```bash
# Run the sine wave test
ros2 run rs_control sine_wave_test.py
```

This will start both motors in sine wave motion with different parameters for 10 seconds.

## Parameters

- `can_interface` (string, default: "can0"): CAN interface name
- `control_frequency` (double, default: 50.0): Control loop frequency in Hz
- `default_kp` (double, default: 2.0): Default position gain
- `default_kd` (double, default: 0.1): Default velocity gain
- `motor_id` (int, default: 1 for single motor nodes): Motor CAN ID 

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
