/*
 * Motor Checker Node for RobStride Motor Diagnostics
 * Scans all motor IDs (0-127), verifies connection, protocol, and state
 */

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/int32.hpp"
#include "std_msgs/msg/string.hpp"

#include <memory>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>

#include "rs_control/can_interface.h"
#include "rs_control/protocol.h"

class MotorCheckerNode : public rclcpp::Node
{
public:
    MotorCheckerNode()
    : Node("motor_checker_node"),
      can_interface_(std::make_unique<CanInterface>())
    {
        // Declare parameters
        this->declare_parameter<std::string>("can_interface", "can0");
        this->declare_parameter<int>("timeout_ms", 100);
        this->declare_parameter<bool>("auto_check", true);
        this->declare_parameter<double>("check_interval", 5.0);
        this->declare_parameter<int>("scan_start_id", 0);
        this->declare_parameter<int>("scan_end_id", 127);

        // Get parameters
        this->get_parameter("can_interface", can_interface_name_);
        this->get_parameter("timeout_ms", timeout_ms_);
        this->get_parameter("auto_check", auto_check_);
        double check_interval = 5.0;
        this->get_parameter("check_interval", check_interval);
        this->get_parameter("scan_start_id", scan_start_id_);
        this->get_parameter("scan_end_id", scan_end_id_);

        // Publishers
        found_motors_pub_ = this->create_publisher<std_msgs::msg::String>("motor/found_motors", 10);
        motor_count_pub_ = this->create_publisher<std_msgs::msg::Int32>("motor/count", 10);
        diagnostic_pub_ = this->create_publisher<std_msgs::msg::String>("motor/diagnostic", 10);

        // Initialize CAN interface
        if (!initialize_can()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to initialize CAN interface");
            return;
        }

        // Run initial scan
        scan_all_motors();

        // Setup periodic checking if enabled
        if (auto_check_) {
            check_timer_ = this->create_wall_timer(
                std::chrono::duration<double>(check_interval),
                std::bind(&MotorCheckerNode::scan_all_motors, this));
        }

        RCLCPP_INFO(this->get_logger(), "Motor Checker Node initialized");
        RCLCPP_INFO(this->get_logger(), "  CAN Interface: %s", can_interface_name_.c_str());
        RCLCPP_INFO(this->get_logger(), "  Scan range: ID %d to %d", scan_start_id_, scan_end_id_);
        RCLCPP_INFO(this->get_logger(), "  Auto-check: %s", auto_check_ ? "enabled" : "disabled");
    }

    ~MotorCheckerNode()
    {
        can_interface_->close();
        RCLCPP_INFO(this->get_logger(), "Motor Checker Node shutdown");
    }

private:
    struct MotorInfo {
        int motor_id = -1;
        int protocol_mode = -1;
        bool connected = false;
        bool motor_enabled = false;
        float vbus = 0.0f;
    };

    bool initialize_can()
    {
        RCLCPP_INFO(this->get_logger(), "Initializing CAN interface: %s", can_interface_name_.c_str());

        if (!can_interface_->init(can_interface_name_)) {
            RCLCPP_ERROR(this->get_logger(), "CAN interface initialization failed");
            return false;
        }

        if (!can_interface_->is_ready()) {
            RCLCPP_ERROR(this->get_logger(), "CAN interface not ready");
            return false;
        }

        RCLCPP_INFO(this->get_logger(), "CAN interface initialized successfully");
        return true;
    }

    // Clear any pending frames in the CAN buffer
    void clear_can_buffer()
    {
        struct can_frame frame;
        while (can_interface_->read_frame(&frame, 1)) {
            // Discard buffered frames
        }
    }

    // Send a broadcast ping to all motors - motors with matching ID will respond
    bool ping_motor(int motor_id)
    {
        // Send GET_DEVICE_ID command
        uint32_t ext_id = (CommType::GET_DEVICE_ID << 24) | (HOST_ID_ << 8) | motor_id;
        uint8_t data[8] = {0};

        if (!can_interface_->send_frame(ext_id, data, 0)) {
            return false;
        }

        // Wait for response
        struct can_frame frame;
        auto start = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeout_ms_);

        while (std::chrono::steady_clock::now() - start < timeout) {
            if (can_interface_->read_frame(&frame, 10)) {
                // Check if response ID matches expected motor_id
                uint8_t response_id = frame.can_id & 0xFF;
                if (response_id == motor_id) {
                    return true;
                }
            }
        }

        return false;
    }

    // Get motor ID from response (for broadcast responses)
    int get_responding_motor_id()
    {
        struct can_frame frame;
        auto start = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeout_ms_);

        while (std::chrono::steady_clock::now() - start < timeout) {
            if (can_interface_->read_frame(&frame, 10)) {
                // Motor ID is in the lower byte of CAN ID
                return static_cast<int>(frame.can_id & 0xFF);
            }
        }
        return -1;
    }

    // Get motor mode for a specific motor
    int get_motor_mode(int motor_id)
    {
        uint32_t ext_id = (CommType::READ_PARAMETER << 24) | (HOST_ID_ << 8) | motor_id;
        uint8_t data[8] = {0};
        pack_u16_le(data, ParamID::MODE);

        if (!can_interface_->send_frame(ext_id, data, 8)) {
            return -1;
        }

        struct can_frame frame;
        auto start = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeout_ms_);

        while (std::chrono::steady_clock::now() - start < timeout) {
            if (can_interface_->read_frame(&frame, 10)) {
                // Check if this response is for our motor
                uint8_t response_id = frame.can_id & 0xFF;
                if (response_id == motor_id && frame.can_dlc >= 3) {
                    return static_cast<int>(frame.data[2]);
                }
            }
        }

        return -1;
    }

    // Check if motor is enabled and get VBUS voltage
    MotorInfo get_motor_info(int motor_id)
    {
        MotorInfo info;
        info.motor_id = motor_id;

        // Send ENABLE command to check responsiveness
        uint32_t ext_id = (CommType::ENABLE << 24) | (HOST_ID_ << 8) | motor_id;
        uint8_t data[8] = {0};
        can_interface_->send_frame(ext_id, data, 0);

        // Read VBUS parameter
        memset(data, 0, 8);
        pack_u16_le(data, ParamID::VBUS);
        ext_id = (CommType::READ_PARAMETER << 24) | (HOST_ID_ << 8) | motor_id;
        can_interface_->send_frame(ext_id, data, 8);

        // Also read MECHANICAL_POSITION to confirm connection
        pack_u16_le(data, ParamID::MECHANICAL_POSITION);
        can_interface_->send_frame(ext_id, data, 8);

        // Wait for responses
        struct can_frame frame;
        auto start = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeout_ms_);
        int responses = 0;
        int max_responses = 3;

        while (std::chrono::steady_clock::now() - start < timeout && responses < max_responses) {
            if (can_interface_->read_frame(&frame, 10)) {
                uint8_t response_id = frame.can_id & 0xFF;
                if (response_id == motor_id) {
                    responses++;
                    info.connected = true;

                    // Parse parameter ID from data (first 2 bytes, little-endian)
                    if (frame.can_dlc >= 3) {
                        uint16_t param_id = static_cast<uint16_t>(frame.data[0]) | (static_cast<uint16_t>(frame.data[1]) << 8);

                        if (param_id == ParamID::VBUS && frame.can_dlc >= 6) {
                            // VBUS is float at data[2:6]
                            float vbus;
                            memcpy(&vbus, &frame.data[2], sizeof(float));
                            info.vbus = vbus;
                        }
                    }
                }
            }
        }

        // Get mode
        info.protocol_mode = get_motor_mode(motor_id);

        // If we got any response, motor is enabled/responsive
        info.motor_enabled = info.connected;

        return info;
    }

    // Scan for a single motor and get its info
    MotorInfo scan_motor(int motor_id)
    {
        MotorInfo info;
        info.motor_id = motor_id;

        // First try to ping the motor
        if (!ping_motor(motor_id)) {
            return info;  // Not found
        }

        info.connected = true;

        // Get detailed info
        MotorInfo detailed_info = get_motor_info(motor_id);
        info.protocol_mode = detailed_info.protocol_mode;
        info.motor_enabled = detailed_info.motor_enabled;
        info.vbus = detailed_info.vbus;

        return info;
    }

    std::vector<MotorInfo> scan_all_motors()
    {
        RCLCPP_INFO(this->get_logger(), "Scanning for motors on CAN bus...");
        std::vector<MotorInfo> found_motors;

        clear_can_buffer();

        for (int motor_id = scan_start_id_; motor_id <= scan_end_id_; motor_id++) {
            MotorInfo info = scan_motor(motor_id);
            if (info.connected) {
                found_motors.push_back(info);
                RCLCPP_INFO(this->get_logger(), "  Found motor ID: %d", motor_id);
            }
        }

        // Publish results
        publish_results(found_motors);

        return found_motors;
    }

    void publish_results(const std::vector<MotorInfo>& motors)
    {
        // Publish motor count
        std_msgs::msg::Int32 count_msg;
        count_msg.data = static_cast<int>(motors.size());
        motor_count_pub_->publish(count_msg);

        // Publish found motors as string list
        std_msgs::msg::String found_msg;
        std::string motor_list;
        for (const auto& motor : motors) {
            if (!motor_list.empty()) {
                motor_list += ",";
            }
            motor_list += std::to_string(motor.motor_id);
        }
        found_msg.data = motor_list;
        found_motors_pub_->publish(found_msg);

        // Publish detailed diagnostic
        std_msgs::msg::String diag_msg;
        std::string status;

        if (motors.empty()) {
            status = "ERROR: No motors found on CAN bus";
        } else {
            status = "Found " + std::to_string(motors.size()) + " motor(s): ";
            for (const auto& motor : motors) {
                status += "ID" + std::to_string(motor.motor_id);

                // Protocol mode
                status += "[";
                switch (motor.protocol_mode) {
                    case ControlMode::MIT_MODE: status += "MIT"; break;
                    case ControlMode::POSITION_MODE: status += "Pos"; break;
                    case ControlMode::SPEED_MODE: status += "Spd"; break;
                    case ControlMode::TORQUE_MODE: status += "Trq"; break;
                    default: status += "Unk"; break;
                }

                status += motor.motor_enabled ? "/ON" : "/OFF";

                if (motor.vbus > 0.0f) {
                    status += "/" + std::to_string(motor.vbus) + "V";
                }

                status += "] ";
            }
        }

        diag_msg.data = status;
        diagnostic_pub_->publish(diag_msg);

        // Log summary
        RCLCPP_INFO(this->get_logger(), "%s", status.c_str());
    }

    // Parameters
    std::string can_interface_name_;
    int timeout_ms_;
    bool auto_check_;
    int scan_start_id_;
    int scan_end_id_;

    // Constants
    static constexpr uint8_t HOST_ID_ = 0xFF;

    // CAN interface
    std::unique_ptr<CanInterface> can_interface_;

    // ROS2 components
    rclcpp::TimerBase::SharedPtr check_timer_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr found_motors_pub_;
    rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr motor_count_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr diagnostic_pub_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<MotorCheckerNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
