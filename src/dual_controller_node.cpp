/*
 * Dual Controller Node for RobStride Motors
 * Simple position control with kp/kd tuning
 */

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

#include <memory>
#include <string>
#include <atomic>
#include <chrono>
#include <thread>

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
        RCLCPP_INFO(this->get_logger(), "Topics: motor1/position, motor1/kp, motor1/kd");
        RCLCPP_INFO(this->get_logger(), "        motor2/position, motor2/kp, motor2/kd");
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

        // Send MIT frames with position, kp, kd
        motor1_->write_mit_frame(pos1_, kp1_, kd1_);
        motor2_->write_mit_frame(pos2_, kp2_, kd2_);

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
    double pos1_ = 0.0, kp1_ = 1.0, kd1_ = 0.1;
    double pos2_ = 0.0, kp2_ = 1.0, kd2_ = 0.1;

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
