/*
 * Full MIT Controller Node for RobStride Motors
 * Implements all parameters of write_mit_frame: position, velocity, kp, kd, torque.
 */

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "std_msgs/msg/int8.hpp"
#include "rs_control/msg/motor_state.hpp"

#include <memory>
#include <string>
#include <atomic>
#include <chrono>
#include <thread>
#include <cmath>

#include "rs_control/rob_stride_motor.h"
#include "rs_control/protocol.h"

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

        // Subscriptions for Motor 1 parameters
        sub_pos1_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor1/position", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) { pos1_ = msg->data; });
        sub_vel1_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor1/velocity", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) { vel1_ = msg->data; });
        sub_kp1_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor1/kp", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) { kp1_ = msg->data; });
        sub_kd1_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor1/kd", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) { kd1_ = msg->data; });
        sub_torque1_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor1/torque", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) { torque1_ = msg->data; });

        // Subscriptions for Motor 2 parameters
        sub_pos2_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor2/position", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) { pos2_ = msg->data; });
        sub_vel2_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor2/velocity", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) { vel2_ = msg->data; });
        sub_kp2_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor2/kp", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) { kp2_ = msg->data; });
        sub_kd2_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor2/kd", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) { kd2_ = msg->data; });
        sub_torque2_ = this->create_subscription<std_msgs::msg::Float64>(
            "motor2/torque", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) { torque2_ = msg->data; });

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

        // Initialize motors
        if (!init_motors()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to initialize motors");
            return;
        }

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
        // Send full MIT frames when in MIT mode
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

        auto msg2 = rs_control::msg::MotorState();
        msg2.header.stamp = this->now();
        msg2.name = {"motor2"};
        msg2.position = {motor2_->get_state().position.load()};
        msg2.velocity = {motor2_->get_state().velocity.load()};
        msg2.torque   = {motor2_->get_state().torque.load()};
        state_pub2_->publish(msg2);
    }

    // Motors
    std::unique_ptr<rs_control::RobStrideMotor> motor1_;
    std::unique_ptr<rs_control::RobStrideMotor> motor2_;

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
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_pos1_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_vel1_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kp1_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kd1_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_torque1_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_pos2_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_vel2_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kp2_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_kd2_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_torque2_;
    rclcpp::Subscription<std_msgs::msg::Int8>::SharedPtr sub_mode1_;
    rclcpp::Subscription<std_msgs::msg::Int8>::SharedPtr sub_mode2_;

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
