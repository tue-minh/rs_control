/*
 * Main Control Node for RobStride Motor Control
 * ROS2 node for controlling RobStride motors via CAN bus
 *
 * Features:
 * - MIT mode position control
 * - Subscribes to target position, kp, kd commands
 * - Publishes motor state feedback
 * - Configurable motor ID and CAN interface
 */

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "std_msgs/msg/int32.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <cmath>
#include <functional>

#include "rs_control/can_interface.h"
#include "rs_control/protocol.h"

class RobStrideControlNode : public rclcpp::Node
{
public:
    RobStrideControlNode()
    : Node("robstride_control_node"),
      motor_id_(1),
      can_interface_(std::make_unique<CanInterface>()),
      running_(true)
    {
        // Declare parameters
        this->declare_parameter<int>("motor_id", 127);
        this->declare_parameter<std::string>("can_interface", "can0");
        this->declare_parameter<double>("control_frequency", 50.0);
        this->declare_parameter<double>("default_kp", 4.0);
        this->declare_parameter<double>("default_kd", 0.5);

        // Get parameters
        this->get_parameter("motor_id", motor_id_);
        this->get_parameter("can_interface", can_interface_name_);
        double control_freq = 50.0;
        this->get_parameter("control_frequency", control_freq);
        this->get_parameter("default_kp", default_kp_);
        this->get_parameter("default_kd", default_kd_);

        // Initialize target values
        target_position_.store(0.0);
        target_kp_.store(default_kp_);
        target_kd_.store(default_kd_);

        // Publishers
        state_publisher_ = this->create_publisher<sensor_msgs::msg::JointState>("motor_state", 10);
        status_publisher_ = this->create_publisher<std_msgs::msg::Int32>("motor_status", 10);

        // Subscribers
        position_subscriber_ = this->create_subscription<std_msgs::msg::Float64>(
            "target_position", 10,
            std::bind(&RobStrideControlNode::position_callback, this, std::placeholders::_1));

        kp_subscriber_ = this->create_subscription<std_msgs::msg::Float64>(
            "target_kp", 10,
            std::bind(&RobStrideControlNode::kp_callback, this, std::placeholders::_1));

        kd_subscriber_ = this->create_subscription<std_msgs::msg::Float64>(
            "target_kd", 10,
            std::bind(&RobStrideControlNode::kd_callback, this, std::placeholders::_1));

        // Initialize CAN and motor
        if (!initialize_can()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to initialize CAN interface");
            return;
        }

        // Start control loop timer (50Hz default)
        control_period_ms_ = static_cast<uint32_t>(1000.0 / control_freq);
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(control_period_ms_),
            std::bind(&RobStrideControlNode::control_loop, this));

        RCLCPP_INFO(this->get_logger(), "RobStride Control Node initialized");
        RCLCPP_INFO(this->get_logger(), "  Motor ID: %d", motor_id_);
        RCLCPP_INFO(this->get_logger(), "  CAN Interface: %s", can_interface_name_.c_str());
        RCLCPP_INFO(this->get_logger(), "  Control Frequency: %.1f Hz", control_freq);
    }

    ~RobStrideControlNode()
    {
        // Disable motor for safety before shutdown
        disable_motor();

        running_.store(false);
        if (control_thread_.joinable()) {
            control_thread_.join();
        }
        can_interface_->close();
        RCLCPP_INFO(this->get_logger(), "RobStride Control Node shutdown");
    }

    void disable_motor()
    {
        if (can_interface_ && can_interface_->is_ready()) {
            RCLCPP_INFO(this->get_logger(), "Disabling motor (safety shutdown)...");
            uint32_t ext_id = (CommType::DISABLE << 24) | (HOST_ID_ << 8) | motor_id_;
            uint8_t data[8] = {0};
            can_interface_->send_frame(ext_id, data, 0);
            // Send multiple times to ensure receipt
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            can_interface_->send_frame(ext_id, data, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            can_interface_->send_frame(ext_id, data, 0);
        }
    }

private:
    bool initialize_can()
    {
        RCLCPP_INFO(this->get_logger(), "Initializing CAN interface: %s", can_interface_name_.c_str());

        if (!can_interface_->init(can_interface_name_)) {
            RCLCPP_ERROR(this->get_logger(), "CAN interface initialization failed");
            return false;
        }

        // Enable motor
        if (!enable_motor()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to enable motor");
            return false;
        }

        // Set MIT mode
        if (!set_motor_mode(ControlMode::MIT_MODE)) {
            RCLCPP_ERROR(this->get_logger(), "Failed to set motor mode");
            return false;
        }

        // Set velocity and torque limits
        if (!set_limits()) {
            RCLCPP_WARN(this->get_logger(), "Failed to set motor limits (non-fatal)");
        }

        RCLCPP_INFO(this->get_logger(), "Motor enabled and configured successfully");
        return true;
    }

    bool enable_motor()
    {
        uint32_t ext_id = (CommType::ENABLE << 24) | (HOST_ID_ << 8) | motor_id_;
        uint8_t data[8] = {0};
        return can_interface_->send_frame(ext_id, data, 0);
    }

    bool set_motor_mode(int8_t mode)
    {
        uint32_t ext_id = (CommType::WRITE_PARAMETER << 24) | (HOST_ID_ << 8) | motor_id_;
        uint8_t data[8] = {0};
        pack_u16_le(data, ParamID::MODE);
        data[4] = static_cast<uint8_t>(mode);
        return can_interface_->send_frame(ext_id, data, 8);
    }

    bool set_limits()
    {
        // Set velocity limit
        uint32_t ext_id = (CommType::WRITE_PARAMETER << 24) | (HOST_ID_ << 8) | motor_id_;
        uint8_t data[8] = {0};
        pack_u16_le(data, ParamID::VELOCITY_LIMIT);
        pack_float_le(&data[4], ModelScale::VELOCITY);
        if (!can_interface_->send_frame(ext_id, data, 8)) {
            return false;
        }

        // Set torque limit
        ext_id = (CommType::WRITE_PARAMETER << 24) | (HOST_ID_ << 8) | motor_id_;
        memset(data, 0, 8);
        pack_u16_le(data, ParamID::TORQUE_LIMIT);
        pack_float_le(&data[4], ModelScale::TORQUE);
        return can_interface_->send_frame(ext_id, data, 8);
    }

    void position_callback(const std_msgs::msg::Float64::SharedPtr msg)
    {
        target_position_.store(msg->data);
        RCLCPP_DEBUG(this->get_logger(), "Received target position: %.4f", msg->data);
    }

    void kp_callback(const std_msgs::msg::Float64::SharedPtr msg)
    {
        target_kp_.store(msg->data);
        RCLCPP_DEBUG(this->get_logger(), "Received target kp: %.4f", msg->data);
    }

    void kd_callback(const std_msgs::msg::Float64::SharedPtr msg)
    {
        target_kd_.store(msg->data);
        RCLCPP_DEBUG(this->get_logger(), "Received target kd: %.4f", msg->data);
    }

    void control_loop()
    {
        if (!running_.load() || !can_interface_->is_ready()) {
            return;
        }

        auto start = std::chrono::steady_clock::now();

        // Send MIT mode control frame
        write_mit_frame();

        // Read and process status frames
        read_status_frames();

        // Publish current state
        publish_state();

        // Timing enforcement
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        auto target_period = std::chrono::microseconds(control_period_ms_ * 1000);

        if (elapsed < target_period) {
            std::this_thread::sleep_for(target_period - elapsed);
        }
    }

    void write_mit_frame()
    {
        double pos = target_position_.load();
        double kp_val = target_kp_.load();
        double kd_val = target_kd_.load();

        // Clamp values to valid ranges
        pos = std::max(-ModelScale::POSITION, std::min(ModelScale::POSITION, pos));
        kp_val = std::max(0.0, std::min(ModelScale::KP, kp_val));
        kd_val = std::max(0.0, std::min(ModelScale::KD, kd_val));

        // Convert to protocol units (big-endian)
        uint16_t pos_u16 = static_cast<uint16_t>(((pos / ModelScale::POSITION) + 1.0) * 0x7FFF);
        uint16_t vel_u16 = 0x7FFF;  // Zero velocity
        uint16_t kp_u16 = static_cast<uint16_t>((kp_val / ModelScale::KP) * 0xFFFF);
        uint16_t kd_u16 = static_cast<uint16_t>((kd_val / ModelScale::KD) * 0xFFFF);
        uint16_t torque_u16 = 0x7FFF;  // Zero feed-forward torque

        uint8_t data[8];
        pack_u16_be(&data[0], pos_u16);
        pack_u16_be(&data[2], vel_u16);
        pack_u16_be(&data[4], kp_u16);
        pack_u16_be(&data[6], kd_u16);

        uint32_t ext_id = (CommType::OPERATION_CONTROL << 24) | (torque_u16 << 8) | motor_id_;
        can_interface_->send_frame(ext_id, data, 8);
    }

    void read_status_frames()
    {
        struct can_frame frame;

        // Read all available status frames
        while (can_interface_->read_frame(&frame, 1)) {
            process_status_frame(frame);
        }
    }

    void process_status_frame(const struct can_frame& frame)
    {
        // Check extended frame
        if (!(frame.can_id & CAN_EFF_FLAG)) {
            return;
        }

        uint32_t comm_type = (frame.can_id >> 24) & 0x1F;

        if (comm_type == CommType::OPERATION_STATUS) {
            // Parse status data (format depends on motor firmware)
            // Standard format: position, velocity, torque (scaled)
            if (frame.can_dlc >= 6) {
                uint16_t pos_raw = (static_cast<uint16_t>(frame.data[0]) << 8) | frame.data[1];
                uint16_t vel_raw = (static_cast<uint16_t>(frame.data[2]) << 8) | frame.data[3];
                uint16_t torque_raw = (static_cast<uint16_t>(frame.data[4]) << 8) | frame.data[5];

                // Convert from protocol units
                current_position_.store(((static_cast<double>(pos_raw) / 0x7FFF) - 1.0) * ModelScale::POSITION);
                current_velocity_.store(((static_cast<double>(vel_raw) / 0x7FFF) - 1.0) * ModelScale::VELOCITY);
                current_torque_.store(((static_cast<double>(torque_raw) / 0x7FFF) - 1.0) * ModelScale::TORQUE);
            }
        }
    }

    void publish_state()
    {
        auto state_msg = sensor_msgs::msg::JointState();
        state_msg.header.stamp = this->now();
        state_msg.name = {"robstride_motor"};
        state_msg.position = {current_position_.load()};
        state_msg.velocity = {current_velocity_.load()};
        state_msg.effort = {current_torque_.load()};
        state_publisher_->publish(state_msg);

        auto status_msg = std_msgs::msg::Int32();
        status_msg.data = can_interface_->is_ready() ? 1 : 0;
        status_publisher_->publish(status_msg);
    }

    // Parameters
    int motor_id_;
    std::string can_interface_name_;
    uint32_t control_period_ms_;
    double default_kp_;
    double default_kd_;

    // Constants
    static constexpr uint8_t HOST_ID_ = 0xFF;

    // CAN interface
    std::unique_ptr<CanInterface> can_interface_;

    // Control targets
    std::atomic<double> target_position_;
    std::atomic<double> target_kp_;
    std::atomic<double> target_kd_;

    // Current state feedback
    std::atomic<double> current_position_;
    std::atomic<double> current_velocity_;
    std::atomic<double> current_torque_;

    // Threading
    std::atomic<bool> running_;
    std::thread control_thread_;

    // ROS2 components
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr state_publisher_;
    rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr status_publisher_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr position_subscriber_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr kp_subscriber_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr kd_subscriber_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<RobStrideControlNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
