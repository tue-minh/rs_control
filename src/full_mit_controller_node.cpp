/*
 * Full MIT Controller Node for RobStride Motors
 * Implements all parameters of write_mit_frame: position, velocity, kp, kd, torque.
 */

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "std_msgs/msg/int8.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"
#include "rs_control/msg/motor_state.hpp"

#include <memory>
#include <string>
#include <atomic>
#include <chrono>
#include <thread>
#include <cmath>

#include "rs_control/rob_stride_motor.h"
#include "rs_control/protocol.h"
#include "rs_control/low_pass_filter.h"

class FullMITControllerNode : public rclcpp::Node
{
public:
    FullMITControllerNode()
    : Node("full_mit_controller_node"),
      motor1_(std::make_unique<rs_control::RobStrideMotor>(1, "can0")),
      motor2_(std::make_unique<rs_control::RobStrideMotor>(127, "can0")),
      running_(true)
    {
        // Publishers for state
        state_pub1_ = this->create_publisher<rs_control::msg::MotorState>("motor1/state", 10);
        state_pub2_ = this->create_publisher<rs_control::msg::MotorState>("motor2/state", 10);
        state_pub1_filtered_ = this->create_publisher<rs_control::msg::MotorState>("motor1/state_filtered", 10);
        state_pub2_filtered_ = this->create_publisher<rs_control::msg::MotorState>("motor2/state_filtered", 10);

        // Mode subscriptions (retain MIT mode handling)
        sub_mode1_ = this->create_subscription<std_msgs::msg::Int8>(
            "motor1/mode", 10,
            [this](const std_msgs::msg::Int8::SharedPtr msg) {
                if (msg->data != mode1_) {
                    mode1_ = msg->data;
                    motor1_->set_mode(mode1_);
                    RCLCPP_INFO(this->get_logger(), "Motor1 mode changed to %d", mode1_);
                }
            });
        sub_mode2_ = this->create_subscription<std_msgs::msg::Int8>(
            "motor2/mode", 10,
            [this](const std_msgs::msg::Int8::SharedPtr msg) {
                if (msg->data != mode2_) {
                    mode2_ = msg->data;
                    motor2_->set_mode(mode2_);
                    RCLCPP_INFO(this->get_logger(), "Motor2 mode changed to %d", mode2_);
                }
            });
        // Matrix configuration subscription for both motors
        sub_cfg_matrix_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "motors/config_matrix", 10,
            [this](const std_msgs::msg::Float64MultiArray::SharedPtr msg) {
                if (msg->data.size() < 10) {
                    RCLCPP_WARN(this->get_logger(), "Config matrix message too short, expected 10 values");
                    return;
                }
                // Motor 1
                pos1_ = msg->data[0];
                vel1_ = msg->data[1];
                kp1_ = msg->data[2];
                kd1_ = msg->data[3];
                torque1_ = msg->data[4];
                // Motor 2
                pos2_ = msg->data[5];
                vel2_ = msg->data[6];
                kp2_ = msg->data[7];
                kd2_ = msg->data[8];
                torque2_ = msg->data[9];
            });

        // Initialize motors
        if (!init_motors()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to initialize motors");
            return;
        }

        // LPF parameter subscriptions (multi-array: [cutoff_freq, sample_time])
        sub_lpf_pos1_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "motor1/lpf_pos/params", 10,
            [this](const std_msgs::msg::Float64MultiArray::SharedPtr msg) {
                if (msg->data.size() < 2) {
                    RCLCPP_WARN(this->get_logger(), "LPF pos1 params too short");
                    return;
                }
                lpf_pos1_.setCutoffFrequency(msg->data[0]);
                lpf_pos1_.setSampleTime(msg->data[1]);
            });
        sub_lpf_vel1_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "motor1/lpf_vel/params", 10,
            [this](const std_msgs::msg::Float64MultiArray::SharedPtr msg) {
                if (msg->data.size() < 2) {
                    RCLCPP_WARN(this->get_logger(), "LPF vel1 params too short");
                    return;
                }
                lpf_vel1_.setCutoffFrequency(msg->data[0]);
                lpf_vel1_.setSampleTime(msg->data[1]);
            });
        sub_lpf_pos2_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "motor2/lpf_pos/params", 10,
            [this](const std_msgs::msg::Float64MultiArray::SharedPtr msg) {
                if (msg->data.size() < 2) {
                    RCLCPP_WARN(this->get_logger(), "LPF pos2 params too short");
                    return;
                }
                lpf_pos2_.setCutoffFrequency(msg->data[0]);
                lpf_pos2_.setSampleTime(msg->data[1]);
            });
        sub_lpf_vel2_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "motor2/lpf_vel/params", 10,
            [this](const std_msgs::msg::Float64MultiArray::SharedPtr msg) {
                if (msg->data.size() < 2) {
                    RCLCPP_WARN(this->get_logger(), "LPF vel2 params too short");
                    return;
                }
                lpf_vel2_.setCutoffFrequency(msg->data[0]);
                lpf_vel2_.setSampleTime(msg->data[1]);
            });

        // Control loop at 50Hz
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(20),
            [this]() { this->control_loop(); });

        RCLCPP_INFO(this->get_logger(), "Full MIT Controller Node started");

    }

    ~FullMITControllerNode()
    {
        running_.store(false);
        if (motor1_) { motor1_->disable(); motor1_->close(); }
        if (motor2_) { motor2_->disable(); motor2_->close(); }
        RCLCPP_INFO(this->get_logger(), "Node shutdown");
    }

private:
    bool init_motors()
    {
        using namespace std::chrono_literals;
        if (!motor1_->initialize() || !motor1_->enable()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to init/enable motor1");
            return false;
        }
        if (!motor2_->initialize() || !motor2_->enable()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to init/enable motor2");
            return false;
        }
        // Give hardware a moment to settle
        std::this_thread::sleep_for(100ms);
        return true;
    }

    void control_loop()
    {
        if (!running_.load()) return;
        // Send full MIT frames when in MIT mode (use original values)
        if (mode1_ == ControlMode::MIT_MODE) {
            motor1_->write_mit_frame(pos1_, vel1_, kp1_, kd1_, torque1_);
        }
        if (mode2_ == ControlMode::MIT_MODE) {
            motor2_->write_mit_frame(pos2_, vel2_, kp2_, kd2_, torque2_);
        }
        // Read status and publish
        motor1_->read_status_frames(1);
        motor2_->read_status_frames(1);
        publish_states();
    }

    void publish_states()
    {
        auto msg1 = rs_control::msg::MotorState();
        msg1.header.stamp = this->now();
        msg1.name = {"motor1"};
        msg1.position = {motor1_->get_state().position.load()};
        msg1.velocity = {motor1_->get_state().velocity.load()};
        msg1.torque   = {motor1_->get_state().torque.load()};
        state_pub1_->publish(msg1);

        auto msg1_f = rs_control::msg::MotorState();
        msg1_f.header.stamp = this->now();
        msg1_f.name = {"motor1"};
        msg1_f.position = {lpf_pos1_.filter(msg1.position[0])};
        msg1_f.velocity = {lpf_vel1_.filter(msg1.velocity[0])};
        msg1_f.torque   = msg1.torque;
        state_pub1_filtered_->publish(msg1_f);

        auto msg2 = rs_control::msg::MotorState();
        msg2.header.stamp = this->now();
        msg2.name = {"motor2"};
        msg2.position = {motor2_->get_state().position.load()};
        msg2.velocity = {motor2_->get_state().velocity.load()};
        msg2.torque   = {motor2_->get_state().torque.load()};
        state_pub2_->publish(msg2);

        auto msg2_f = rs_control::msg::MotorState();
        msg2_f.header.stamp = this->now();
        msg2_f.name = {"motor2"};
        msg2_f.position = {lpf_pos2_.filter(msg2.position[0])};
        msg2_f.velocity = {lpf_vel2_.filter(msg2.velocity[0])};
        msg2_f.torque   = msg2.torque;
        state_pub2_filtered_->publish(msg2_f);
    }

    // Motors
    std::unique_ptr<rs_control::RobStrideMotor> motor1_;
    std::unique_ptr<rs_control::RobStrideMotor> motor2_;

    // Low-pass filters for position and velocity
    rs_control::LowPassFilter lpf_pos1_{10.0, 0.02};
    rs_control::LowPassFilter lpf_vel1_{10.0, 0.02};
    rs_control::LowPassFilter lpf_pos2_{10.0, 0.02};
    rs_control::LowPassFilter lpf_vel2_{10.0, 0.02};

    // Current control values for motor 1
    double pos1_ = 0.0, vel1_ = 0.0, kp1_ = 2.0, kd1_ = 0.5, torque1_ = 0.0;
    // Current control values for motor 2
    double pos2_ = 0.0, vel2_ = 0.0, kp2_ = 2.0, kd2_ = 0.5, torque2_ = 0.0;

    // Mode values (default to MIT)
    int8_t mode1_ = ControlMode::MIT_MODE;
    int8_t mode2_ = ControlMode::MIT_MODE;

// ROS interfaces
     rclcpp::TimerBase::SharedPtr timer_;
     rclcpp::Publisher<rs_control::msg::MotorState>::SharedPtr state_pub1_;
     rclcpp::Publisher<rs_control::msg::MotorState>::SharedPtr state_pub2_;
     rclcpp::Publisher<rs_control::msg::MotorState>::SharedPtr state_pub1_filtered_;
     rclcpp::Publisher<rs_control::msg::MotorState>::SharedPtr state_pub2_filtered_;
     rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_pos1_;
     rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_vel1_;
     rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kp1_;
     rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kd1_;
     rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_torque1_;
     rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_pos2_;
     rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_vel2_;
     rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kp2_;
     rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kd2_;
     rclcpp::Subscription<std_msgs::msg::Int8>::SharedPtr sub_mode1_;
     rclcpp::Subscription<std_msgs::msg::Int8>::SharedPtr sub_mode2_;
     rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_cfg_matrix_;
     // LPF parameter parameters (multi-array: [cutoff_freq, sample_time])
     rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_lpf_pos1_;
     rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_lpf_vel1_;
     rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_lpf_pos2_;
     rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_lpf_vel2_;

    std::atomic<bool> running_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<FullMITControllerNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
