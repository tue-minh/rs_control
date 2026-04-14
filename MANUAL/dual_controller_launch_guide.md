# Dual Controller Launch Guide

## Overview
The `dual_controller_with_logging.launch.py` launch file starts the dual controller node for 2 RobStride motors, logs all motor topics to the console, and creates a Foxglove Bridge for visualization.

## Prerequisites

### 1. Install Foxglove Bridge
```bash
# Install the Foxglove Bridge ROS 2 package
sudo apt install ros-$ROS_DISTRO-foxglove-bridge
```

Or build from source:
```bash
cd ~/ros2_ws/src
git clone https://github.com/foxglove/ros-foxglove-bridge.git
cd ~/ros2_ws
colcon build --packages-select foxglove_bridge
source install/setup.bash
```

### 2. Setup CAN Interface
Ensure your CAN interface is properly configured:
```bash
# Bring up CAN interface
sudo ip link set can0 up type can bitrate 1000000

# Verify interface is up
ip -details link show can0
```

## Usage

### 1. Build the Package
```bash
cd ~/ros2_ws
colcon build --packages-select rs_control
source install/setup.bash
```

### 2. Launch Everything
```bash
ros2 launch rs_control dual_controller_with_logging.launch.py
```

This will start:
- ✅ **dual_controller_node**: Controls motor1 (ID: 1) and motor2 (ID: 127)
- ✅ **Topic echo loggers**: Logs motor states and commands to console
- ✅ **foxglove_bridge**: WebSocket server on port 8765

### 3. Connect Foxglove Studio

1. Open **Foxglove Studio**
2. Click **"Open Connection"**
3. Select **"Foxglove WebSocket"**
4. Enter connection details:
   - **Host**: `localhost` (or your machine's IP if remote)
   - **Port**: `8765`
5. Click **"Open"**

### 4. Configure Foxglove Panels

Once connected, add the following panels:

#### Panel 1: Motor 1 State
- Add a **Plot** panel
- Subscribe to: `/motor1/state`
- Display: `position[0]`, `velocity[0]`

#### Panel 2: Motor 2 State
- Add a **Plot** panel
- Subscribe to: `/motor2/state`
- Display: `position[0]`, `velocity[0]`

#### Panel 3: Send Commands
- Add a **Publish** panel
- Publish to topics:
  - `/motor1/position` (std_msgs/Float64)
  - `/motor2/position` (std_msgs/Float64)
  - `/motor1/kp` (std_msgs/Float64)
  - `/motor2/kp` (std_msgs/Float64)
  - `/motor1/kd` (std_msgs/Float64)
  - `/motor2/kd` (std_msgs/Float64)

## Available Topics

### Motor 1
- `/motor1/state` - Joint state (position, velocity)
- `/motor1/position` - Position command
- `/motor1/kp` - Proportional gain
- `/motor1/kd` - Derivative gain

### Motor 2
- `/motor2/state` - Joint state (position, velocity)
- `/motor2/position` - Position command
- `/motor2/kp` - Proportional gain
- `/motor2/kd` - Derivative gain

## Testing Commands

In a separate terminal, you can send commands:

```bash
# Set motor 1 position
ros2 topic pub /motor1/position std_msgs/msg/Float64 "{data: 1.5}"

# Set motor 2 position
ros2 topic pub /motor2/position std_msgs/msg/Float64 "{data: -1.0}"

# Adjust KP for motor 1
ros2 topic pub /motor1/kp std_msgs/msg/Float64 "{data: 5.0}"

# Adjust KD for motor 2
ros2 topic pub /motor2/kd std_msgs/msg/Float64 "{data: 0.5}"
```

## Troubleshooting

### Foxglove Bridge Not Found
```bash
# Check if bridge is running
ros2 node list | grep foxglove

# Check if port is open
netstat -tuln | grep 8765
```

### CAN Interface Issues
```bash
# Check CAN interface status
ip -details link show can0

# Restart CAN interface
sudo ip link set can0 down
sudo ip link set can0 up type can bitrate 1000000
```

### Motor Not Responding
- Check motor ID configuration in the launch file
- Verify CAN bus wiring and termination
- Check console output for error messages

## Stopping

Press `Ctrl+C` in the launch terminal to stop all nodes.
