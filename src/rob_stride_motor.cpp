/*
 * RobStride Motor Library Implementation
 * High-level CAN bus and motor control interface
 */

#include "rs_control/rob_stride_motor.h"
#include "rs_control/protocol.h"

#include <cstring>
#include <algorithm>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <iostream>

namespace rs_control {

// --- Constants ---
namespace {
    constexpr double POSITION_SCALE = 4.0 * M_PI;  // ±4π radians
    constexpr double VELOCITY_SCALE = 50.0;
    constexpr double TORQUE_SCALE = 60.0;
    constexpr double KP_SCALE = 5000.0;
    constexpr double KD_SCALE = 100.0;
} // anonymous namespace

// --- RobStrideMotor Implementation ---

RobStrideMotor::RobStrideMotor(int motor_id, const std::string& can_interface)
    : motor_id_(motor_id)
    , can_interface_(can_interface)
    , socket_fd_(-1)
{
}

RobStrideMotor::~RobStrideMotor() {
    close();
}

bool RobStrideMotor::initialize() {
    // Create socket
    socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd_ < 0) {
        std::cerr << "RobStrideMotor: socket creation failed" << std::endl;
        return false;
    }

    // Get interface index
    struct ifreq ifr;
    strcpy(ifr.ifr_name, can_interface_.c_str());
    if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "RobStrideMotor: ioctl failed for interface " << can_interface_ << std::endl;
        close();
        return false;
    }

    // Bind socket to interface
    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(socket_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "RobStrideMotor: bind failed" << std::endl;
        close();
        return false;
    }

    // Enable motor
    if (!enable()) {
        std::cerr << "RobStrideMotor: failed to enable motor" << std::endl;
        return false;
    }

    // Set MIT mode
    if (!set_mode(ControlMode::MIT_MODE)) {
        std::cerr << "RobStrideMotor: failed to set MIT mode" << std::endl;
        return false;
    }

    return true;
}

void RobStrideMotor::close() {
    if (socket_fd_ >= 0) {
        ::close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool RobStrideMotor::is_ready() const {
    return socket_fd_ >= 0;
}

bool RobStrideMotor::enable() {
    uint32_t ext_id = build_ext_id(CommType::ENABLE);
    uint8_t data[8] = {0};
    return send_frame(ext_id, data, 0);
}

bool RobStrideMotor::disable() {
    uint32_t ext_id = build_ext_id(CommType::DISABLE);
    uint8_t data[8] = {0};
    bool ret = send_frame(ext_id, data, 0);
    // Send multiple times to ensure receipt
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    send_frame(ext_id, data, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    send_frame(ext_id, data, 0);
    return ret;
}

bool RobStrideMotor::set_mode(int8_t mode) {
    uint32_t ext_id = build_ext_id(CommType::WRITE_PARAMETER);
    uint8_t data[8] = {0};
    pack_u16_le(data, ParamID::MODE);
    data[4] = static_cast<uint8_t>(mode);
    return send_frame(ext_id, data, 8);
}

bool RobStrideMotor::write_parameter(uint16_t param_id, float value) {
    uint32_t ext_id = build_ext_id(CommType::WRITE_PARAMETER);
    uint8_t data[8] = {0};
    pack_u16_le(data, param_id);
    pack_float_le(&data[4], value);
    return send_frame(ext_id, data, 8);
}

bool RobStrideMotor::read_parameter(uint16_t param_id, float& value, int timeout_ms) {
    uint32_t ext_id = build_ext_id(CommType::READ_PARAMETER);
    uint8_t data[8] = {0};
    pack_u16_le(data, param_id);

    if (!send_frame(ext_id, data, 8)) {
        return false;
    }

    struct can_frame frame;
    auto end_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(end_time - now);
        if (remaining.count() <= 0) {
            break;
        }

        struct timeval tv;
        tv.tv_sec = remaining.count() / 1000000;
        tv.tv_usec = remaining.count() % 1000000;

        fd_set rdfs;
        FD_ZERO(&rdfs);
        FD_SET(socket_fd_, &rdfs);

        int ret = select(socket_fd_ + 1, &rdfs, nullptr, nullptr, &tv);
        if (ret <= 0) {
            break;
        }

        ssize_t nbytes = read(socket_fd_, &frame, sizeof(struct can_frame));
        if (nbytes != sizeof(struct can_frame)) {
            break;
        }

        // Check if this is a parameter response for our motor
        uint8_t frame_motor_id = (frame.can_id >> 8) & 0xFF;
        if (frame_motor_id != motor_id_) {
            continue;
        }

        uint32_t comm_type = (frame.can_id >> 24) & 0x1F;
        if (comm_type == CommType::READ_PARAMETER && frame.can_dlc >= 6) {
            // Check if param_id matches
            uint16_t resp_param_id = static_cast<uint16_t>(frame.data[0]) | (static_cast<uint16_t>(frame.data[1]) << 8);
            if (resp_param_id == param_id) {
                memcpy(&value, &frame.data[4], sizeof(float));
                return true;
            }
        }
    }

    return false;
}

bool RobStrideMotor::write_position_frame(double position) {
    // Use OPERATION_CONTROL command for position mode control
    uint32_t ext_id = (CommType::OPERATION_CONTROL << 24) | (0 << 8) | motor_id_;
    uint8_t data[8] = {0};
    float pos_float = static_cast<float>(position);
    pack_float_le(&data[0], pos_float);
    // data[4-7] can be used for velocity/torque feedforward if needed
    return send_frame(ext_id, data, 8);
}

bool RobStrideMotor::write_velocity_frame(double velocity) {
    // Use OPERATION_CONTROL command for velocity mode control
    // Similar to MIT frame format but velocity is in bytes 2-3
    uint32_t ext_id = (CommType::OPERATION_CONTROL << 24) | (0 << 8) | motor_id_;
    uint8_t data[8] = {0};

    // Clamp velocity to valid range
    velocity = std::max(-VELOCITY_SCALE, std::min(VELOCITY_SCALE, velocity));

    // Convert velocity to protocol units (big-endian, same as MIT)
    uint16_t vel_u16 = static_cast<uint16_t>(((velocity / VELOCITY_SCALE) + 1.0) * 0x7FFF);

    // Position field = 0x7FFF (center/neutral)
    pack_u16_be(&data[0], 0x7FFF);
    // Velocity field = target velocity
    pack_u16_be(&data[2], vel_u16);
    // KP and KD fields = 0 for simple velocity control
    pack_u16_be(&data[4], 0);
    pack_u16_be(&data[6], 0);

    return send_frame(ext_id, data, 8);
}

bool RobStrideMotor::set_limits(double velocity_limit, double torque_limit) {
    // Set velocity limit
    uint32_t ext_id = build_ext_id(CommType::WRITE_PARAMETER);
    uint8_t data[8] = {0};
    pack_u16_le(data, ParamID::VELOCITY_LIMIT);
    pack_float_le(&data[4], static_cast<float>(velocity_limit));
    if (!send_frame(ext_id, data, 8)) {
        return false;
    }

    // Set torque limit
    ext_id = build_ext_id(CommType::WRITE_PARAMETER);
    memset(data, 0, 8);
    pack_u16_le(data, ParamID::TORQUE_LIMIT);
    pack_float_le(&data[4], static_cast<float>(torque_limit));
    return send_frame(ext_id, data, 8);
}

bool RobStrideMotor::write_mit_frame(double position, double kp, double kd, double torque) {
    // Clamp values to valid ranges
    position = clamp_position(position);
    kp = clamp_kp(kp);
    kd = clamp_kd(kd);

    // Convert to protocol units (big-endian)
    uint16_t pos_u16 = static_cast<uint16_t>(((position / POSITION_SCALE) + 1.0) * 0x7FFF);
    uint16_t vel_u16 = 0x7FFF;  // Zero velocity
    uint16_t kp_u16 = static_cast<uint16_t>((kp / KP_SCALE) * 0xFFFF);
    uint16_t kd_u16 = static_cast<uint16_t>((kd / KD_SCALE) * 0xFFFF);
    uint16_t torque_u16 = static_cast<uint16_t>(((torque / TORQUE_SCALE) + 1.0) * 0x7FFF);

    uint8_t data[8];
    pack_u16_be(&data[0], pos_u16);
    pack_u16_be(&data[2], vel_u16);
    pack_u16_be(&data[4], kp_u16);
    pack_u16_be(&data[6], kd_u16);

    uint32_t ext_id = (CommType::OPERATION_CONTROL << 24) | (torque_u16 << 8) | motor_id_;
    return send_frame(ext_id, data, 8);
}

int RobStrideMotor::read_status_frames(int timeout_ms) {
    if (socket_fd_ < 0) {
        return 0;
    }

    struct can_frame frame;
    int count = 0;

    // Read all available status frames with total timeout
    auto end_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

    while (true) {
        // Calculate remaining time
        auto now = std::chrono::steady_clock::now();
        auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(end_time - now);
        if (remaining.count() <= 0) {
            break;  // Timeout
        }

        struct timeval tv;
        tv.tv_sec = remaining.count() / 1000000;
        tv.tv_usec = remaining.count() % 1000000;

        fd_set rdfs;
        FD_ZERO(&rdfs);
        FD_SET(socket_fd_, &rdfs);

        int ret = select(socket_fd_ + 1, &rdfs, nullptr, nullptr, &tv);
        if (ret <= 0) {
            break;  // Timeout or error
        }

        ssize_t nbytes = read(socket_fd_, &frame, sizeof(struct can_frame));
        if (nbytes != sizeof(struct can_frame)) {
            break;
        }

        process_status_frame(frame);
        count++;
    }

    return count;
}

void RobStrideMotor::process_status_frame(const struct can_frame& frame) {
    // Check extended frame
    if (!(frame.can_id & CAN_EFF_FLAG)) {
        return;
    }

    // Extract motor ID from CAN ID (bits 8-15)
    uint8_t frame_motor_id = (frame.can_id >> 8) & 0xFF;
    if (frame_motor_id != motor_id_) {
        return;  // This frame is for a different motor
    }

    uint32_t comm_type = (frame.can_id >> 24) & 0x1F;

    if (comm_type == CommType::OPERATION_STATUS) {
        // Parse status data
        if (frame.can_dlc >= 6) {
            uint16_t pos_raw = (static_cast<uint16_t>(frame.data[0]) << 8) | frame.data[1];
            uint16_t vel_raw = (static_cast<uint16_t>(frame.data[2]) << 8) | frame.data[3];
            uint16_t torque_raw = (static_cast<uint16_t>(frame.data[4]) << 8) | frame.data[5];

            // Convert from protocol units
            state_.position.store(((static_cast<double>(pos_raw) / 0x7FFF) - 1.0) * POSITION_SCALE);
            state_.velocity.store(((static_cast<double>(vel_raw) / 0x7FFF) - 1.0) * VELOCITY_SCALE);
            state_.torque.store(((static_cast<double>(torque_raw) / 0x7FFF) - 1.0) * TORQUE_SCALE);

            // Call callback if set
            if (status_callback_) {
                status_callback_(state_);
            }
        }
    }
}

bool RobStrideMotor::send_frame(uint32_t can_id, const uint8_t* data, uint8_t dlc) {
    if (socket_fd_ < 0) {
        return false;
    }

    struct can_frame frame;
    frame.can_id = can_id | CAN_EFF_FLAG;  // Extended frame format
    frame.can_dlc = dlc;

    if (data && dlc > 0) {
        memcpy(frame.data, data, dlc);
    } else {
        memset(frame.data, 0, 8);
    }

    ssize_t nbytes = write(socket_fd_, &frame, sizeof(struct can_frame));
    return nbytes == sizeof(struct can_frame);
}

uint32_t RobStrideMotor::build_ext_id(uint32_t comm_type, uint16_t) {
    return (comm_type << 24) | (HOST_ID_ << 8) | motor_id_;
}

// --- Static Utility Functions ---

double RobStrideMotor::clamp_position(double pos) {
    return std::max(-POSITION_SCALE, std::min(POSITION_SCALE, pos));
}

double RobStrideMotor::clamp_kp(double kp) {
    return std::max(0.0, std::min(KP_SCALE, kp));
}

double RobStrideMotor::clamp_kd(double kd) {
    return std::max(0.0, std::min(KD_SCALE, kd));
}

} // namespace rs_control