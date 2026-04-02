/*
 * Motor State Reader Node for RobStride Motors
 * ROS2 node for reading RobStride motor state via CAN bus
 *
 * Features:
 * - Read-only node (no control commands)
 * - Publishes position, velocity, torque feedback
 * - Publishes motor connection status
 * - Configurable motor ID and CAN interface
 */

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/int32.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <cmath>

#include "rs_control/can_interface.h"
#include "rs_control/protocol.h"

class MotorStateReaderNode : public rclcpp::Node
{
public:
    MotorStateReaderNode()
    : Node("motor_state_reader_node"),
      motor_id_(1),
      can_interface_(std::make_unique<CanInterface>()),
      running_(true)
    {
        // Declare parameters
        this->declare_parameter<int>("motor_id", 127);
        this->declare_parameter<std::string>("can_interface", "can0");
        this->declare_parameter<double>("read_frequency", 50.0);

        // Get parameters
        this->get_parameter("motor_id", motor_id_);
        this->get_parameter("can_interface", can_interface_name_);
        double read_freq = 50.0;
        this->get_parameter("read_frequency", read_freq);

        // Publishers
        state_publisher_ = this->create_publisher<sensor_msgs::msg::JointState>("motor_state", 10);
        status_publisher_ = this->create_publisher<std_msgs::msg::Int32>("motor_status", 10);

        // Initialize CAN interface
        if (!initialize_can()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to initialize CAN interface");
            return;
        }

        // Start read loop timer
        read_period_ms_ = static_cast<uint32_t>(1000.0 / read_freq);
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(read_period_ms_),
            std::bind(&MotorStateReaderNode::read_loop, this));

        RCLCPP_INFO(this->get_logger(), "Motor State Reader Node initialized");
        RCLCPP_INFO(this->get_logger(), "  Motor ID: %d", motor_id_);
        RCLCPP_INFO(this->get_logger(), "  CAN Interface: %s", can_interface_name_.c_str());
        RCLCPP_INFO(this->get_logger(), "  Read Frequency: %.1f Hz", read_freq);
    }

    ~MotorStateReaderNode()
    {
        running_.store(false);
        can_interface_->close();
        RCLCPP_INFO(this->get_logger(), "Motor State Reader Node shutdown");
    }

private:
    bool initialize_can()
    {
        RCLCPP_INFO(this->get_logger(), "Initializing CAN interface: %s", can_interface_name_.c_str());

        if (!can_interface_->init(can_interface_name_)) {
            RCLCPP_ERROR(this->get_logger(), "CAN interface initialization failed");
            return false;
        }

        RCLCPP_INFO(this->get_logger(), "CAN interface initialized successfully");
        return true;
    }

    void read_loop()
    {
        if (!running_.load() || !can_interface_->is_ready()) {
            return;
        }

        // Read all available status frames
        read_status_frames();

        // Publish current state
        publish_state();
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
    uint32_t read_period_ms_;

    // CAN interface
    std::unique_ptr<CanInterface> can_interface_;

    // Current state feedback
    std::atomic<double> current_position_;
    std::atomic<double> current_velocity_;
    std::atomic<double> current_torque_;

    // Threading
    std::atomic<bool> running_;

    // ROS2 components
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr state_publisher_;
    rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr status_publisher_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<MotorStateReaderNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
