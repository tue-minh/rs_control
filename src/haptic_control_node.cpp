/*
 * Haptic Control Node for 2 RobStride Motors
 * Master: Motor ID 127 - user moves it, reads position, receives torque feedback
 * Slave:  Motor ID 1 - follows master position, provides force feedback
 */

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"
#include "rs_control/msg/motor_state.hpp"

#include <memory>
#include <atomic>
#include <chrono>
#include <thread>
#include <cmath>

#include "rs_control/rob_stride_motor.h"
#include "rs_control/protocol.h"

class HapticControlNode : public rclcpp::Node
{
public:
    HapticControlNode()
    : Node("haptic_control_node"),
      master_(std::make_unique<rs_control::RobStrideMotor>(127, "can0")),
      slave_(std::make_unique<rs_control::RobStrideMotor>(1, "can0")),
      running_(true)
    {
        // Publishers - master state
        master_state_pub_ = this->create_publisher<rs_control::msg::MotorState>("master/state", 10);
        // Publishers - slave state
        slave_state_pub_ = this->create_publisher<rs_control::msg::MotorState>("slave/state", 10);

        // Master position publisher (for external use)
        master_pos_pub_ = this->create_publisher<std_msgs::msg::Float64>("master/position", 10);

        // Slave torque feedback publisher
        slave_torque_pub_ = this->create_publisher<std_msgs::msg::Float64>("slave/torque_feedback", 10);

        // Subscription: external position command for slave (overrides master position)
        sub_slave_pos_cmd_ = this->create_subscription<std_msgs::msg::Float64>(
            "slave/position_cmd", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                slave_pos_cmd_ = msg->data;
                use_cmd_ = true;
            });

        // Subscription: master torque feedforward
        sub_master_torque_ff_ = this->create_subscription<std_msgs::msg::Float64>(
            "master/torque_ff", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                master_torque_ff_ = msg->data;
            });

        // Matrix configuration subscription: kp, kd, torque for both motors
        // Format: [master_kp, master_kd, master_torque_ff, slave_kp, slave_kd, slave_torque]
        sub_cfg_matrix_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "haptic/config_matrix", 10,
            [this](const std_msgs::msg::Float64MultiArray::SharedPtr msg) {
                if (msg->data.size() < 6) {
                    RCLCPP_WARN(this->get_logger(), "Config matrix too short, expected 6 values");
                    return;
                }
                master_kp_ = msg->data[0];
                master_kd_ = msg->data[1];
                master_torque_ff_ = msg->data[2];
                slave_kp_ = msg->data[3];
                slave_kd_ = msg->data[4];
                slave_torque_ = msg->data[5];
                RCLCPP_INFO(this->get_logger(), "Config updated: M[kp=%.2f kd=%.2f t=%.2f] S[kp=%.2f kd=%.2f t=%.2f]",
                    master_kp_, master_kd_, master_torque_ff_.load(),
                    slave_kp_, slave_kd_, slave_torque_);
            });

        // Initialize motors
        if (!init_motors()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to initialize motors");
            return;
        }

        // Control loop at 100Hz
        timer_ = this->create_wall_timer(
            // std::chrono::milliseconds(10),
            std::chrono::milliseconds(5), //200hz
            [this]() { this->control_loop(); });

        RCLCPP_INFO(this->get_logger(), "Haptic Control Node started (Master=127, Slave=1)");
    }

    ~HapticControlNode()
    {
        running_.store(false);
        if (master_) { master_->disable(); master_->close(); }
        if (slave_) { slave_->disable(); slave_->close(); }
        RCLCPP_INFO(this->get_logger(), "Node shutdown");
    }

private:
    bool init_motors()
    {
        using namespace std::chrono_literals;

        RCLCPP_INFO(this->get_logger(), "Initializing master motor (ID 127)...");
        if (!master_->initialize() || !master_->enable()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to init/enable master motor");
            return false;
        }

        RCLCPP_INFO(this->get_logger(), "Initializing slave motor (ID 1)...");
        if (!slave_->initialize() || !slave_->enable()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to init/enable slave motor");
            return false;
        }

        // Set both to MIT mode
        master_->set_mode(ControlMode::MIT_MODE);
        slave_->set_mode(ControlMode::MIT_MODE);

        std::this_thread::sleep_for(100ms);
        return true;
    }

    void control_loop()
    {
        if (!running_.load()) return;

        // 1. Read motor states
        master_->read_status_frames(1);
        slave_->read_status_frames(1);

        double master_pos = master_->get_state().position.load();
        double master_vel = master_->get_state().velocity.load();
        double master_torque = master_->get_state().torque.load();

        double slave_pos = slave_->get_state().position.load();
        double slave_vel = slave_->get_state().velocity.load();
        double slave_torque = slave_->get_state().torque.load();

        // 2. Determine slave target position
        double slave_target = use_cmd_.load() ? slave_pos_cmd_.load() : master_pos;

        // 3. Control slave: track master position (or external cmd)
        slave_->write_mit_frame(slave_target, 0.0, slave_kp_, slave_kd_, slave_torque_);

        // 4. Force feedback: apply slave torque to master (negative = resistance)
        double feedback_torque = -slave_torque + master_torque_ff_.load();
        // master_->write_mit_frame(master_pos, 0.0, master_kp_, master_kd_, feedback_torque);
        master_->write_mit_frame(master_pos, 0.0, 0, master_kd_, feedback_torque);

        // 5. Publish states
        publish_states(master_pos, master_vel, master_torque, slave_pos, slave_vel, slave_torque);

        // 6. Publish master position
        auto pos_msg = std_msgs::msg::Float64();
        pos_msg.data = master_pos;
        master_pos_pub_->publish(pos_msg);

        // 7. Publish slave torque feedback
        auto torque_msg = std_msgs::msg::Float64();
        torque_msg.data = slave_torque;
        slave_torque_pub_->publish(torque_msg);
    }

    void publish_states(double m_pos, double m_vel, double m_torque,
                        double s_pos, double s_vel, double s_torque)
    {
        auto master_msg = rs_control::msg::MotorState();
        master_msg.header.stamp = this->now();
        master_msg.name = {"master"};
        master_msg.position = {m_pos};
        master_msg.velocity = {m_vel};
        master_msg.torque = {m_torque};
        master_state_pub_->publish(master_msg);

        auto slave_msg = rs_control::msg::MotorState();
        slave_msg.header.stamp = this->now();
        slave_msg.name = {"slave"};
        slave_msg.position = {s_pos};
        slave_msg.velocity = {s_vel};
        slave_msg.torque = {s_torque};
        slave_state_pub_->publish(slave_msg);
    }

    // Motors
    std::unique_ptr<rs_control::RobStrideMotor> master_;
    std::unique_ptr<rs_control::RobStrideMotor> slave_;

    // Control parameters
    std::atomic<double> slave_pos_cmd_{0.0};
    std::atomic<double> master_torque_ff_{0.0};
    std::atomic<bool> use_cmd_{false};

    // Tunable gains (updated via config_matrix)
    double master_kp_{0.0};
    double master_kd_{0.02};
    double slave_kp_{3};
    double slave_kd_{0.5};
    double slave_torque_{0.0};

    // ROS interfaces
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<rs_control::msg::MotorState>::SharedPtr master_state_pub_;
    rclcpp::Publisher<rs_control::msg::MotorState>::SharedPtr slave_state_pub_;
    rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr master_pos_pub_;
    rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr slave_torque_pub_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_slave_pos_cmd_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr sub_master_torque_ff_;
    rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_cfg_matrix_;

    std::atomic<bool> running_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HapticControlNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
