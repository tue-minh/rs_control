/*
 * Dual Controller Node for RobStride Motors
 * Simple position control with kp/kd tuning
 */

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "std_msgs/msg/int8.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

#include <memory>
#include <string>
#include <atomic>
#include <chrono>
#include <thread>
#include <cmath>

#include "rs_control/rob_stride_motor.h"
#include "rs_control/protocol.h"

class DualControllerNode : public rclcpp::Node
{
public:
    DualControllerNode()
    : Node("dual_controller_node"),
      motor1_(std::make_unique<rs_control::RobStrideMotor>(1, "can0")),
      motor2_(std::make_unique<rs_control::RobStrideMotor>(127, "can0")),
      running_(true)
    {
        // Publishers
        state_pub1_ = this->create_publisher<sensor_msgs::msg::JointState>("motor1/state", 10);
        state_pub2_ = this->create_publisher<sensor_msgs::msg::JointState>("motor2/state", 10);

        // Subscribers for Motor 1
        sub_pos1_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor1/position", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                pos1_ = msg->data;
            });

        sub_kp1_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor1/kp", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                kp1_ = msg->data;
            });

        sub_kd1_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor1/kd", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                kd1_ = msg->data;
            });

        // Subscribers for Motor 2
        sub_pos2_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor2/position", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                pos2_ = msg->data;
            });

        sub_kp2_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor2/kp", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                kp2_ = msg->data;
            });

        sub_kd2_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor2/kd", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                kd2_ = msg->data;
            });

        // Sinewave subscribers for Motor 1 (used in SINEWAVE_MODE)
        sub_amp1_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor1/amp", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                amp1_ = msg->data;
            });

        sub_freq1_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor1/freq", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                freq1_ = msg->data;
            });

        // Sinewave subscribers for Motor 2 (used in SINEWAVE_MODE)
        sub_amp2_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor2/amp", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                amp2_ = msg->data;
            });

        sub_freq2_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor2/freq", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                freq2_ = msg->data;
            });

        // Mode subscribers for Motor 1
        sub_mode1_ = this->create_subscription<std_msgs::msg::Int8>(
            "motor1/mode", 10,
            [this](const std_msgs::msg::Int8::SharedPtr msg) {
                if (msg->data != mode1_) {
                    mode1_ = msg->data;
                    motor1_->set_mode(mode1_);
                    if (mode1_ == ControlMode::SINEWAVE_MODE) {
                        // Reset sinewave start time
                        start_time1_ = this->now();
                        // Store current position as center
                        center1_ = motor1_->get_state().position.load();
                    }
                    RCLCPP_INFO(this->get_logger(), "Motor1 mode changed to %d", mode1_);
                }
            });

        // Mode subscribers for Motor 2
        sub_mode2_ = this->create_subscription<std_msgs::msg::Int8>(
            "motor2/mode", 10,
            [this](const std_msgs::msg::Int8::SharedPtr msg) {
                if (msg->data != mode2_) {
                    mode2_ = msg->data;
                    motor2_->set_mode(mode2_);
                    if (mode2_ == ControlMode::SINEWAVE_MODE) {
                        start_time2_ = this->now();
                        center2_ = motor2_->get_state().position.load();
                    }
                    RCLCPP_INFO(this->get_logger(), "Motor2 mode changed to %d", mode2_);
                }
            });

        // Initialize
        if (!init_motors()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to initialize motors");
            return;
        }

        // Control loop at 50Hz
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(20),
            [this]() { this->control_loop(); });

        RCLCPP_INFO(this->get_logger(), "Dual Controller Node started");
        RCLCPP_INFO(this->get_logger(), "Topics: motor1/position, motor1/kp, motor1/kd, motor1/mode");
        RCLCPP_INFO(this->get_logger(), "        motor2/position, motor2/kp, motor2/kd, motor2/mode");
        RCLCPP_INFO(this->get_logger(), "Modes: 0=MIT, 1=Position, 2=Sinewave, 3=Torque");
        RCLCPP_INFO(this->get_logger(), "Sinewave (mode 2): amp=2, freq=0.5, vel_limit=5 by default");
    }

    ~DualControllerNode()
    {
        running_.store(false);
        if (motor1_) {
            motor1_->disable();
            motor1_->close();
        }
        if (motor2_) {
            motor2_->disable();
            motor2_->close();
        }
        RCLCPP_INFO(this->get_logger(), "Node shutdown");
    }

private:
    bool init_motors()
    {
        using namespace std::chrono_literals;

        // Init Motor 1
        if (!motor1_->initialize()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to init motor1");
            return false;
        }
        if (!motor1_->enable()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to enable motor1");
            return false;
        }
        std::this_thread::sleep_for(100ms);

        // Init Motor 2
        if (!motor2_->initialize()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to init motor2");
            return false;
        }
        if (!motor2_->enable()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to enable motor2");
            return false;
        }
        std::this_thread::sleep_for(100ms);

        return true;
    }

    void control_loop()
    {
        if (!running_.load()) return;

        // Get current time for sinewave calculation
        auto now = this->now();
        double t1 = (now.seconds() - start_time1_.seconds());
        double t2 = (now.seconds() - start_time2_.seconds());

        // Send control frames based on mode
        if (mode1_ == ControlMode::MIT_MODE) {
            motor1_->write_mit_frame(pos1_, kp1_, kd1_);
        } else if (mode1_ == ControlMode::SINEWAVE_MODE) {
            // Sinewave: position = center + amp * sin(2*pi*freq*t)
            double pos_sine1 = center1_ + amp1_ * std::sin(2.0 * M_PI * freq1_ * t1);
            // Clamp velocity to vel_limit_
            double vel1 = amp1_ * 2.0 * M_PI * freq1_ * std::cos(2.0 * M_PI * freq1_ * t1);
            vel1 = std::max(-vel_limit_, std::min(vel_limit_, vel1));
            // Use MIT frame with velocity clamping via kd
            motor1_->write_mit_frame(pos_sine1, kp1_, kd1_);
        } else {
            motor1_->write_position_frame(pos1_);
        }

        if (mode2_ == ControlMode::MIT_MODE) {
            motor2_->write_mit_frame(pos2_, kp2_, kd2_);
        } else if (mode2_ == ControlMode::SINEWAVE_MODE) {
            double pos_sine2 = center2_ + amp2_ * std::sin(2.0 * M_PI * freq2_ * t2);
            double vel2 = amp2_ * 2.0 * M_PI * freq2_ * std::cos(2.0 * M_PI * freq2_ * t2);
            vel2 = std::max(-vel_limit_, std::min(vel_limit_, vel2));
            motor2_->write_mit_frame(pos_sine2, kp2_, kd2_);
        } else {
            motor2_->write_position_frame(pos2_);
        }

        // Read states
        motor1_->read_status_frames(1);
        motor2_->read_status_frames(1);

        // Publish states
        publish_states();
    }

    void publish_states()
    {
        auto msg1 = sensor_msgs::msg::JointState();
        msg1.header.stamp = this->now();
        msg1.name = {"motor1"};
        msg1.position = {motor1_->get_state().position.load()};
        msg1.velocity = {motor1_->get_state().velocity.load()};
        state_pub1_->publish(msg1);

        auto msg2 = sensor_msgs::msg::JointState();
        msg2.header.stamp = this->now();
        msg2.name = {"motor2"};
        msg2.position = {motor2_->get_state().position.load()};
        msg2.velocity = {motor2_->get_state().velocity.load()};
        state_pub2_->publish(msg2);
    }

    // Motors
    std::unique_ptr<rs_control::RobStrideMotor> motor1_;
    std::unique_ptr<rs_control::RobStrideMotor> motor2_;

    // Control values
    double pos1_ = 0.0, kp1_ = 2.0, kd1_ = 0.5;
    double pos2_ = 0.0, kp2_ = 2.0, kd2_ = 0.5;
    double amp1_ = 2.0, freq1_ = 0.2;  // Sinewave params for motor1
    double amp2_ = 2.0, freq2_ = 0.2;  // Sinewave params for motor2
    double center1_ = 0.0, center2_ = 0.0;  // Center position for sinewave
    double vel_limit_ = 5.0;  // Velocity limit for sinewave mode
    rclcpp::Time start_time1_;  // Start time for motor1 sinewave
    rclcpp::Time start_time2_;  // Start time for motor2 sinewave
    int8_t mode1_ = ControlMode::MIT_MODE;
    int8_t mode2_ = ControlMode::MIT_MODE;

    // ROS
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr state_pub1_;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr state_pub2_;

    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_pos1_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kp1_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kd1_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_pos2_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kp2_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kd2_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_amp1_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_freq1_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_amp2_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_freq2_;
    rclcpp::Subscription<std_msgs::msg::Int8>::SharedPtr sub_mode1_;
    rclcpp::Subscription<std_msgs::msg::Int8>::SharedPtr sub_mode2_;

    std::atomic<bool> running_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<DualControllerNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
