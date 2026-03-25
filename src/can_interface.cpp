/*
 * CAN Interface Implementation for RobStride Motor Control
 * SocketCAN interface wrapper
 */

#include "rs_control/can_interface.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

CanInterface::CanInterface()
    : socket_fd_(-1)
{
}

CanInterface::~CanInterface()
{
    close();
}

bool CanInterface::init(const std::string& interface)
{
    // Create socket
    socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd_ < 0) {
        std::cerr << "CanInterface: socket creation failed" << std::endl;
        return false;
    }

    // Get interface index
    struct ifreq ifr;
    strcpy(ifr.ifr_name, interface.c_str());
    if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "CanInterface: ioctl failed for interface " << interface << std::endl;
        close();
        return false;
    }

    // Bind socket to interface
    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(socket_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "CanInterface: bind failed" << std::endl;
        close();
        return false;
    }

    interface_name_ = interface;
    return true;
}

void CanInterface::close()
{
    if (socket_fd_ >= 0) {
        ::close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool CanInterface::send_frame(uint32_t can_id, const uint8_t* data, uint8_t dlc)
{
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

bool CanInterface::read_frame(struct can_frame* frame, int timeout_ms)
{
    if (socket_fd_ < 0 || frame == nullptr) {
        return false;
    }

    // Set timeout
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    fd_set rdfs;
    FD_ZERO(&rdfs);
    FD_SET(socket_fd_, &rdfs);

    int ret = select(socket_fd_ + 1, &rdfs, nullptr, nullptr, &tv);
    if (ret <= 0) {
        return false;  // Timeout or error
    }

    ssize_t nbytes = read(socket_fd_, frame, sizeof(struct can_frame));
    return nbytes == sizeof(struct can_frame);
}
