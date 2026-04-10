/*
 * Dual Motor Control Node for RobStride Motors
 * ROS2 node for controlling 2 RobStride motors (RS-03 ID:1, RS-06 ID:127)
 *
 * Features:
 * - Sine wave motion control for each motor independently
 * - Topics: /sine_wave_rs03, /sine_wave_rs06
 * - Parameters per motor: amplitude, frequency, kp, kd, velocity_limit
 */

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

#include <memory>
#include <string>
#include <atomic>
#include <cmath>
#include <chrono>

#include "rs_control/rob_stride_motor.h"

// Message for sine wave parameters
struct SineWaveParams {
    double amplitude = 0.0;
    double frequency = 0.0;
    double kp = 2.0;
    double kd = 0.1;
    double velocity_limit = 0.0;
    bool enabled = false;
};

class DualMotorControlNode : public rclcpp::Node
{
public:
    DualMotorControlNode()
    : Node("dual_motor_control_node"),
      motor_rs03_(std::make_unique<rs_control::RobStrideMotor>(1, "can0")),
      motor_rs06_(std::make_unique<rs_control::RobStrideMotor>(127, "can0")),
      running_(true)
    {
        // Get parameters
        double control_freq = 50.0;
        this->get_parameter_or("control_frequency", control_freq, 50.0);

        // Initialize sine wave params
        sine_params_rs03_.amplitude = 1.0;
        sine_params_rs03_.frequency = 0.5;
        sine_params_rs03_.kp = 2.0;
        sine_params_rs03_.kd = 0.1;
        sine_params_rs03_.velocity_limit = 5.0;

        sine_params_rs06_.amplitude = 1.0;
        sine_params_rs06_.frequency = 0.5;
        sine_params_rs06_.kp = 2.0;
        sine_params_rs06_.kd = 0.1;
        sine_params_rs06_.velocity_limit = 5.0;

        start_time_ = std::chrono::steady_clock::now();

        // Publishers
        state_publisher_rs03_ = this->create_publisher<sensor_msgs::msg::JointState>("motor_rs03_state", 10);
        state_publisher_rs06_ = this->create_publisher<sensor_msgs::msg::JointState>("motor_rs06_state", 10);

        // Subscribers for RS-03 sine wave parameters
        sub_amp_rs03_ = this->create_subscription<std_msgs::msg::Float64>(
            "sine_wave_rs03/amplitude", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                sine_params_rs03_.amplitude = msg->data;
                sine_params_rs03_.enabled = true;
            });

        sub_freq_rs03_ = this->create_subscription<std_msgs::msg::Float64>(
            "sine_wave_rs03/frequency", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                sine_params_rs03_.frequency = msg->data;
                sine_params_rs03_.enabled = true;
            });

        sub_kp_rs03_ = this->create_subscription<std_msgs::msg::Float64>(
            "sine_wave_rs03/kp", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                sine_params_rs03_.kp = msg->data;
            });

        sub_kd_rs03_ = this->create_subscription<std_msgs::msg::Float64>(
            "sine_wave_rs03/kd", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                sine_params_rs03_.kd = msg->data;
            });

        sub_vel_rs03_ = this->create_subscription<std_msgs::msg::Float64>(
            "sine_wave_rs03/velocity_limit", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                sine_params_rs03_.velocity_limit = msg->data;
            });

        // Subscribers for RS-06 sine wave parameters
        sub_amp_rs06_ = this->create_subscription<std_msgs::msg::Float64>(
            "sine_wave_rs06/amplitude", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                sine_params_rs06_.amplitude = msg->data;
                sine_params_rs06_.enabled = true;
            });

        sub_freq_rs06_ = this->create_subscription<std_msgs::msg::Float64>(
            "sine_wave_rs06/frequency", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                sine_params_rs06_.frequency = msg->data;
                sine_params_rs06_.enabled = true;
            });

        sub_kp_rs06_ = this->create_subscription<std_msgs::msg::Float64>(
            "sine_wave_rs06/kp", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                sine_params_rs06_.kp = msg->data;
            });

        sub_kd_rs06_ = this->create_subscription<std_msgs::msg::Float64>(
            "sine_wave_rs06/kd", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                sine_params_rs06_.kd = msg->data;
            });

        sub_vel_rs06_ = this->create_subscription<std_msgs::msg::Float64>(
            "sine_wave_rs06/velocity_limit", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                sine_params_rs06_.velocity_limit = msg->data;
            });

        // Initialize motors
        if (!initialize_motors()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to initialize motors");
            return;
        }

        // Control loop timer
        control_period_ms_ = static_cast<uint32_t>(1000.0 / control_freq);
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(control_period_ms_),
            [this]() { this->control_loop(); });

        RCLCPP_INFO(this->get_logger(), "Dual Motor Control Node initialized");
        RCLCPP_INFO(this->get_logger(), "  RS-03 (ID: 1) topics: sine_wave_rs03/amplitude, frequency, kp, kd, velocity_limit");
        RCLCPP_INFO(this->get_logger(), "  RS-06 (ID: 127) topics: sine_wave_rs06/amplitude, frequency, kp, kd, velocity_limit");
    }

    ~DualMotorControlNode()
    {
        running_.store(false);
        if (motor_rs03_) motor_rs03_->close();
        if (motor_rs06_) motor_rs06_->close();
        RCLCPP_INFO(this->get_logger(), "Dual Motor Control Node shutdown");
    }

private:
    bool initialize_motors()
    {
        RCLCPP_INFO(this->get_logger(), "Initializing RS-03 (ID: 1)...");
        if (!motor_rs03_->initialize()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to initialize RS-03");
            return false;
        }

        RCLCPP_INFO(this->get_logger(), "Initializing RS-06 (ID: 127)...");
        if (!motor_rs06_->initialize()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to initialize RS-06");
            return false;
        }

        RCLCPP_INFO(this->get_logger(), "Both motors initialized");
        return true;
    }

    double calculate_sine_position(const SineWaveParams& params, double time)
    {
        if (!params.enabled || params.frequency <= 0.0) {
            return 0.0;
        }

        double amplitude = params.amplitude;
        double frequency = params.frequency;

        // Apply velocity limit by scaling amplitude
        double max_velocity = amplitude * 2.0 * M_PI * frequency;
        if (params.velocity_limit > 0.0 && max_velocity > params.velocity_limit) {
            amplitude = params.velocity_limit / (2.0 * M_PI * frequency);
        }

        return amplitude * std::sin(2.0 * M_PI * frequency * time);
    }

    void control_loop()
    {
        if (!running_.load()) return;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - start_time_);
        double time_sec = elapsed.count();

        // Calculate sine wave positions
        double pos_rs03 = calculate_sine_position(sine_params_rs03_, time_sec);
        double pos_rs06 = calculate_sine_position(sine_params_rs06_, time_sec);

        // Send MIT frames
        motor_rs03_->write_mit_frame(pos_rs03, sine_params_rs03_.kp, sine_params_rs03_.kd);
        motor_rs06_->write_mit_frame(pos_rs06, sine_params_rs06_.kp, sine_params_rs06_.kd);

        // Read motor states
        motor_rs03_->read_status_frames(1);
        motor_rs06_->read_status_frames(1);

        // Publish states
        publish_states();
    }

    void publish_states()
    {
        const auto& state_rs03 = motor_rs03_->get_state();
        auto msg_rs03 = sensor_msgs::msg::JointState();
        msg_rs03.header.stamp = this->now();
        msg_rs03.name = {"robstride_rs03"};
        msg_rs03.position = {state_rs03.position.load()};
        msg_rs03.velocity = {state_rs03.velocity.load()};
        state_publisher_rs03_->publish(msg_rs03);

        const auto& state_rs06 = motor_rs06_->get_state();
        auto msg_rs06 = sensor_msgs::msg::JointState();
        msg_rs06.header.stamp = this->now();
        msg_rs06.name = {"robstride_rs06"};
        msg_rs06.position = {state_rs06.position.load()};
        msg_rs06.velocity = {state_rs06.velocity.load()};
        state_publisher_rs06_->publish(msg_rs06);
    }

    // Motors
    std::unique_ptr<rs_control::RobStrideMotor> motor_rs03_;
    std::unique_ptr<rs_control::RobStrideMotor> motor_rs06_;

    // Sine wave parameters
    SineWaveParams sine_params_rs03_;
    SineWaveParams sine_params_rs06_;

    // Timing
    std::chrono::steady_clock::time_point start_time_;
    uint32_t control_period_ms_;
    std::atomic<bool> running_;

    // ROS components
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr state_publisher_rs03_;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr state_publisher_rs06_;

    // RS-03 subscribers
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_amp_rs03_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_freq_rs03_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kp_rs03_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kd_rs03_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_vel_rs03_;

    // RS-06 subscribers
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_amp_rs06_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_freq_rs06_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kp_rs06_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kd_rs06_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_vel_rs06_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<DualMotorControlNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
