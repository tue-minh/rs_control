/*
 * RobStride Motor Library
 * High-level CAN bus and motor control interface for RobStride motors
 *
 * Features:
 * - MIT mode position control
 * - Motor enable/disable
 * - Status frame processing
 * - Configurable CAN interface and motor ID
 */

#ifndef ROB_STRIDE_MOTOR_H
#define ROB_STRIDE_MOTOR_H

#include <cstdint>
#include <string>
#include <atomic>
#include <memory>
#include <functional>

// Forward declarations
struct can_frame;

namespace rs_control {

// --- Motor State ---
struct MotorState {
    std::atomic<double> position{0.0};
    std::atomic<double> velocity{0.0};
    std::atomic<double> torque{0.0};
};

// --- Callback Types ---
using StatusCallback = std::function<void(const MotorState&)>;

/**
 * @brief RobStrideMotor class for CAN-based motor control
 */
class RobStrideMotor {
public:
    /**
     * @brief Construct a RobStrideMotor
     * @param motor_id Motor ID (default: 1)
     * @param can_interface CAN interface name (default: "can0")
     */
    RobStrideMotor(int motor_id = 1, const std::string& can_interface = "can0");
    ~RobStrideMotor();

    // Disable copy
    RobStrideMotor(const RobStrideMotor&) = delete;
    RobStrideMotor& operator=(const RobStrideMotor&) = delete;

    /**
     * @brief Initialize the motor (CAN interface + motor setup)
     * @return true if successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Close CAN interface and cleanup
     */
    void close();

    /**
     * @brief Check if motor is initialized and ready
     */
    bool is_ready() const;

    /**
     * @brief Enable the motor
     */
    bool enable();

    /**
     * @brief Disable the motor (safety shutdown)
     */
    bool disable();

    /**
     * @brief Set motor control mode
     * @param mode Mode value (0=MIT, 1=Position, 2=Speed, 3=Torque)
     */
    bool set_mode(int8_t mode);

    /**
     * @brief Write a parameter to the motor
     * @param param_id Parameter ID (see ParamID namespace)
     * @param value Parameter value as float
     */
    bool write_parameter(uint16_t param_id, float value);

    /**
     * @brief Read a parameter from the motor
     * @param param_id Parameter ID (see ParamID namespace)
     * @param value Output parameter for the read value
     * @param timeout_ms Read timeout in milliseconds
     * @return true if successful, false otherwise
     */
    bool read_parameter(uint16_t param_id, float& value, int timeout_ms = 100);

    /**
     * @brief Write position mode frame
     * @param position Target position (radians)
     */
    bool write_position_frame(double position);

    /**
     * @brief Set velocity and torque limits
     * @param velocity_limit Velocity limit (default: 50.0 rad/s)
     * @param torque_limit Torque limit (default: 60.0 Nm)
     */
    bool set_limits(double velocity_limit = 50.0, double torque_limit = 60.0);

    /**
     * @brief Send MIT mode control frame
     * @param position Target position (radians)
     * @param kp Position stiffness (0-5000 Nm/rad)
     * @param kd Damping (0-100 Nm/rad/s)
     * @param torque Feed-forward torque (optional, default: 0)
     */
    bool write_mit_frame(double position, double kp, double kd, double torque = 0.0);

    /**
     * @brief Read and process available status frames
     * @param timeout_ms Read timeout in milliseconds
     * @return number of frames processed
     */
    int read_status_frames(int timeout_ms = 1);

    /**
     * @brief Process a single status frame
     */
    void process_status_frame(const struct can_frame& frame);

    /**
     * @brief Get current motor state
     */
    const MotorState& get_state() const { return state_; }

    /**
     * @brief Set status callback (called when new state is received)
     */
    void set_status_callback(StatusCallback callback) { status_callback_ = std::move(callback); }

    // --- Static utility functions ---
    static double clamp_position(double pos);
    static double clamp_kp(double kp);
    static double clamp_kd(double kd);

private:
    int motor_id_;
    std::string can_interface_;
    int socket_fd_;

    MotorState state_;
    StatusCallback status_callback_;

    static constexpr uint8_t HOST_ID_ = 0xFF;

    // Helper methods
    bool send_frame(uint32_t can_id, const uint8_t* data, uint8_t dlc);
    uint32_t build_ext_id(uint32_t comm_type, uint16_t torque = 0);
};

} // namespace rs_control

#endif // ROB_STRIDE_MOTOR_H