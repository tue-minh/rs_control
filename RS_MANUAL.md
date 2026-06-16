# RobStride Motor Control Manual

## Table of Contents
1. [Overview](#overview)
2. [CAN Frame Protocol](#can-frame-protocol)
3. [Control Modes](#control-modes)
4. [API Reference](#api-reference)
5. [Usage Examples](#usage-examples)
6. [Troubleshooting](#troubleshooting)

---

## Overview

This library provides ROS2-compatible control for RobStride RS-03 motors via CAN bus. The motors support multiple control modes for different applications including position control, velocity control, and MIT (impedance) mode for compliant control.

**Supported Motor**: RobStride RS-03  
**CAN Interface**: `can0` (configurable)  
**Control Frequency**: Recommended 50-100Hz

---

## CAN Frame Protocol

### Extended CAN ID Format

```
[31-24]: Communication Type (8-bit)
[23-16]: Torque/Value High Byte (8-bit)  
[15-8] : Reserved (8-bit)
[7-0]  : Motor ID (8-bit)
```

### Communication Types

| ID  | Name                  | Description                        |
|-----|-----------------------|------------------------------------|
| 0x00| GET_DEVICE_ID         | Request motor ID                  |
| 0x01| OPERATION_CONTROL     | Motion control command            |
| 0x02| OPERATION_STATUS      | Motor status feedback             |
| 0x03| ENABLE                | Enable motor                      |
| 0x04| DISABLE               | Disable motor                     |
| 0x06| SET_ZERO_POSITION    | Set mechanical zero position      |
| 0x07| SET_DEVICE_ID         | Change motor CAN ID              |
| 0x11| READ_PARAMETER        | Read motor parameter              |
| 0x12| WRITE_PARAMETER       | Write motor parameter             |
| 0x15| FAULT_REPORT          | Error feedback                    |
| 0x16| SAVE_PARAMETERS       | Save parameters to flash          |

### Parameter IDs (for READ/WRITE)

| ID     | Name                  | Description                    | Access |
|--------|-----------------------|--------------------------------|--------|
| 0x2005 | MECHANICAL_OFFSET     | Mechanical offset              | R/W    |
| 0x3016 | MEASURED_POSITION     | Measured position (rad)        | R      |
| 0x3017 | MEASURED_VELOCITY     | Measured velocity (rad/s)      | R      |
| 0x302C | MEASURED_TORQUE       | Measured torque (Nm)           | R      |
| 0x7005 | MODE                  | Control mode                   | R/W    |
| 0x7006 | IQ_TARGET             | Q-axis current target (A)     | R/W    |
| 0x700A | VELOCITY_TARGET       | Velocity target (rad/s)         | R/W    |
| 0x700B | TORQUE_LIMIT          | Torque limit (Nm)              | R/W    |
| 0x7010 | CURRENT_KP            | Current loop Kp                | R/W    |
| 0x7011 | CURRENT_KI            | Current loop Ki                | R/W    |
| 0x7014 | CURRENT_FILTER_GAIN   | Current filter gain            | R/W    |
| 0x7016 | POSITION_TARGET       | Position target (rad)          | R/W    |
| 0x7017 | VELOCITY_LIMIT        | Velocity limit (rad/s)         | R/W    |
| 0x7018 | CURRENT_LIMIT         | Current limit (A)              | R/W    |
| 0x7019 | MECHANICAL_POSITION   | Mechanical position (rad)      | R      |
| 0x701A | IQ_FILTERED           | Filtered Q-axis current (A)    | R      |
| 0x701B | MECHANICAL_VELOCITY   | Mechanical velocity (rad/s)    | R      |
| 0x701C | VBUS                  | Bus voltage (V)                | R      |
| 0x701E | POSITION_KP           | Position loop Kp                | R/W    |
| 0x701F | VELOCITY_KP           | Velocity loop Kp                | R/W    |
| 0x7020 | VELOCITY_KI           | Velocity loop Ki                | R/W    |

### Model Scaling (RS-03)

| Parameter | Range    | Scaling Factor |
|-----------|----------|----------------|
| Position  | ±4π rad  | 4.0 * π        |
| Velocity  | ±50 rad/s| 50.0           |
| Torque    | ±60 Nm   | 60.0           |
| Kp        | 0-5000   | 5000.0         |
| Kd        | 0-100    | 100.0          |

---

## Control Modes

### Mode 0: MIT Mode (Impedance Control)

**Best for**: Compliant control, impedance control, human-robot interaction

MIT mode implements a PD controller with feedforward torque:
```
torque = Kp * (target_position - current_position) + Kd * (target_velocity - current_velocity) + feedforward_torque
```

**CAN Frame Format**:
```
Data[0-1]: Position    (uint16, big-endian, 0x0000 = -4π, 0x7FFF = 0, 0xFFFF = +4π)
Data[2-3]: Velocity    (uint16, big-endian, 0x0000 = -50 rad/s, 0x7FFF = 0, 0xFFFF = +50 rad/s)
Data[4-5]: Kp          (uint16, big-endian, 0x0000 = 0, 0xFFFF = 5000 Nm/rad)
Data[6-7]: Kd          (uint16, big-endian, 0x0000 = 0, 0xFFFF = 100 Nm/rad/s)
Extended ID[16-31]: Torque Feedforward (uint16, same mapping as velocity)
```

**Control Parameters**:
| Parameter | Range        | Description |
|-----------|--------------|-------------|
| position  | ±4π rad      | Target position |
| velocity  | ±50 rad/s    | Target velocity (for damping) |
| kp        | 0-5000       | Position stiffness (Nm/rad) |
| kd        | 0-100        | Damping coefficient (Nm/rad/s) |
| torque    | ±60 Nm       | Feedforward torque |

**Usage Tips**:
- Set Kp=0, Kd=0 for pure torque control
- Set Kp high, Kd low for position control with some compliance
- Set Kp high, Kd high for stiff position control
- Use torque feedforward for gravity compensation

---

### Mode 1: Position Mode

**Best for**: Precise position control, trajectory tracking

Uses internal PID position controller.

**CAN Frame Format**:
```
Extended ID: 0x01 << 24 | Motor_ID
Data[0-3]: Position (float, radians)
Data[4-7]: Velocity limit (float, rad/s)
```

**Required Parameters** (via write_parameter):
- `ParamID::POSITION_TARGET` (0x7016): Target position
- `ParamID::VELOCITY_LIMIT` (0x7017): Maximum velocity

---

### Mode 2: Velocity Mode (Sinewave Mode)

**Best for**: Speed control, sinusoidal motion profiles

**CAN Frame Format**:
```
Extended ID: 0x01 << 24 | Motor_ID
Data[0-3]: Position reference (float)
Data[4-7]: Unused
```

**Required Parameters** (via write_parameter):
- `ParamID::VELOCITY_TARGET` (0x700A): Target velocity

---

### Mode 3: Torque Mode

**Best for**: Direct torque control, torque-based applications

**CAN Frame Format**:
```
Data[0-3]: IQ_TARGET (float, -23 to +23 A)
```

**Required Parameters**:
- `ParamID::IQ_TARGET` (0x7006): Q-axis current reference

---

### Mode 4: Sinewave Mode (MIT-based)

**Best for**: Oscillatory motion, testing

An extension of MIT mode where position follows a sinewave trajectory. The controller calculates:
```
position = center + amplitude * sin(2π * frequency * t)
```

---

## API Reference

### RobStrideMotor Class

```cpp
#include "rs_control/rob_stride_motor.h"
```

#### Constructor

```cpp
RobStrideMotor(int motor_id = 1, const std::string& can_interface = "can0");
```

#### Initialization

```cpp
bool initialize();
```
Initializes CAN socket, enables motor, and sets MIT mode. Returns true on success.

```cpp
void close();
```
Closes CAN socket.

```cpp
bool is_ready() const;
```
Returns true if motor is initialized.

#### Motor Control

```cpp
bool enable();
```
Enables the motor (must be called after initialization).

```cpp
bool disable();
```
Disables the motor (safety shutdown).

```cpp
bool set_mode(int8_t mode);
```
Sets control mode:
- `ControlMode::MIT_MODE` (0)
- `ControlMode::POSITION_MODE` (1)
- `ControlMode::SPEED_MODE` (2)
- `ControlMode::TORQUE_MODE` (3)
- `ControlMode::SINEWAVE_MODE` (4)

#### Parameter Access

```cpp
bool write_parameter(uint16_t param_id, float value);
bool read_parameter(uint16_t param_id, float& value, int timeout_ms = 100);
```
Read/write motor parameters (see Parameter IDs table).

```cpp
bool set_limits(double velocity_limit = 50.0, double torque_limit = 60.0);
```
Set velocity and torque limits.

#### Control Frames

```cpp
bool write_mit_frame(double position, double kp, double kd, double torque = 0.0);
bool write_mit_frame(double position, double velocity, double kp, double kd, double torque = 0.0);
```
**MIT mode control** - the primary method for impedance control.

The first overload uses zero velocity target. The second overload includes velocity control for damping-based velocity control.

```cpp
bool write_position_frame(double position);
```
Position mode control.

```cpp
bool write_velocity_frame(double velocity);
```
Velocity mode control.

#### State Reading

```cpp
int read_status_frames(int timeout_ms = 1);
```
Read and process available status frames. Returns number of frames processed.

```cpp
void process_status_frame(const struct can_frame& frame);
```
Process a single status frame.

```cpp
const MotorState& get_state() const;
```
Get current motor state (position, velocity, torque).

```cpp
void set_status_callback(StatusCallback callback);
```
Set callback for state updates.

#### Utility Functions

```cpp
static double clamp_position(double pos);
static double clamp_kp(double kp);
static double clamp_kd(double kd);
```
Clamp values to valid ranges.

---

## Usage Examples

### Example 1: Basic MIT Mode (Position Control with Compliance)

```cpp
#include "rs_control/rob_stride_motor.h"

int main() {
    rs_control::RobStrideMotor motor(1, "can0");
    
    if (!motor.initialize()) {
        std::cerr << "Failed to initialize motor" << std::endl;
        return 1;
    }
    
    // MIT mode: target position = 0, Kp=100, Kd=5, no feedforward torque
    motor.write_mit_frame(0.0, 100.0, 5.0, 0.0);
    
    // Loop
    while (true) {
        motor.write_mit_frame(1.57, 100.0, 5.0);  // Target 90 degrees
        motor.read_status_frames(1);
        
        auto state = motor.get_state();
        std::cout << "Position: " << state.position 
                  << ", Velocity: " << state.velocity 
                  << ", Torque: " << state.torque << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    motor.disable();
    motor.close();
    return 0;
}
```

### Example 2: MIT Mode with Torque Control

```cpp
// Pure torque control: Kp=0, Kd=0
motor.write_mit_frame(0.0, 0.0, 0.0, 5.0);  // Apply 5 Nm torque

// Position + torque feedforward (for gravity compensation)
motor.write_mit_frame(1.57, 50.0, 2.0, 2.0);  // Target 90 deg with 2 Nm feedforward
```

### Example 3: MIT Mode with Velocity Control

```cpp
// To control velocity in MIT mode, use Kd with a non-zero target velocity:
// torque = Kp*(target_pos - current_pos) + Kd*(target_vel - current_vel) + feedforward
// This effectively makes the motor follow a velocity trajectory

double target_pos = 0.0;    // Keep position near zero
double kp = 10.0;          // Low stiffness
double kd = 50.0;          // High damping for velocity control
double target_vel = 10.0;  // Target velocity in rad/s

// Note: Current implementation has velocity in the frame but motor uses it differently
// For explicit velocity control, use Position Mode or set appropriate Kp/Kd
```

### Example 4: ROS2 Node with Dual Motor Control

```cpp
// See src/dual_controller_node.cpp for complete example
// Topics:
//   motor1/position, motor1/kp, motor1/kd - MIT mode parameters
//   motor1/mode - Control mode (0=MIT, 1=Position, 2=Sinewave, 3=Torque)
//   motor1/amp, motor1/freq - Sinewave parameters
```

---

## Troubleshooting

### Motor Not Responding

1. Check CAN bus connection
2. Verify motor ID matches
3. Ensure motor is powered
4. Check if motor needs enable command

### Position Overshoot

- Increase Kd (damping)
- Decrease Kp (stiffness)
- Reduce control loop delay

### Motor Vibrates

- Kp too high
- Kd too low
- Control frequency too low (minimum 50Hz recommended)

### Cannot Read Parameters

- Increase timeout_ms
- Check CAN wiring
- Verify motor supports parameter

### Torque/Velocity Not Working in MIT Mode

**This is a known issue in the original implementation.** The fix is:

In `write_mit_frame()`, the velocity and torque parameters were not being used correctly. The corrected implementation should:

1. Use velocity in data bytes 2-3 (not hardcoded to zero)
2. Use torque properly - it can be in Extended ID or as feedforward

See the fixed implementation in `src/rob_stride_motor.cpp`.

---

## Protocol Constants (protocol.h)

```cpp
namespace CommType {
    constexpr uint32_t GET_DEVICE_ID       = 0;
    constexpr uint32_t OPERATION_CONTROL   = 1;
    constexpr uint32_t OPERATION_STATUS    = 2;
    constexpr uint32_t ENABLE              = 3;
    constexpr uint32_t DISABLE             = 4;
    constexpr uint32_t SET_ZERO_POSITION   = 6;
    constexpr uint32_t SET_DEVICE_ID       = 7;
    constexpr uint32_t READ_PARAMETER      = 17;
    constexpr uint32_t WRITE_PARAMETER     = 18;
}

namespace ControlMode {
    constexpr int8_t MIT_MODE        = 0;
    constexpr int8_t POSITION_MODE   = 1;
    constexpr int8_t SPEED_MODE      = 2;
    constexpr int8_t TORQUE_MODE     = 3;
    constexpr int8_t SINEWAVE_MODE   = 4;
}

namespace ModelScale {
    constexpr double POSITION = 4.0 * M_PI;  // ±4π radians
    constexpr double VELOCITY = 50.0;        // ±50 rad/s
    constexpr double TORQUE = 60.0;          // ±60 Nm
    constexpr double KP = 5000.0;            // 0-5000 Nm/rad
    constexpr double KD = 100.0;            // 0-100 Nm/rad/s
}
```