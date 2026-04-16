// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from rs_control:msg/SineWaveParameters.idl
// generated code does not contain a copyright notice

#ifndef RS_CONTROL__MSG__DETAIL__SINE_WAVE_PARAMETERS__STRUCT_HPP_
#define RS_CONTROL__MSG__DETAIL__SINE_WAVE_PARAMETERS__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__rs_control__msg__SineWaveParameters __attribute__((deprecated))
#else
# define DEPRECATED__rs_control__msg__SineWaveParameters __declspec(deprecated)
#endif

namespace rs_control
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct SineWaveParameters_
{
  using Type = SineWaveParameters_<ContainerAllocator>;

  explicit SineWaveParameters_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->amplitude = 0.0;
      this->frequency = 0.0;
      this->velocity_limit = 0.0;
      this->kp = 0.0;
      this->kd = 0.0;
    }
  }

  explicit SineWaveParameters_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_alloc;
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->amplitude = 0.0;
      this->frequency = 0.0;
      this->velocity_limit = 0.0;
      this->kp = 0.0;
      this->kd = 0.0;
    }
  }

  // field types and members
  using _amplitude_type =
    double;
  _amplitude_type amplitude;
  using _frequency_type =
    double;
  _frequency_type frequency;
  using _velocity_limit_type =
    double;
  _velocity_limit_type velocity_limit;
  using _kp_type =
    double;
  _kp_type kp;
  using _kd_type =
    double;
  _kd_type kd;

  // setters for named parameter idiom
  Type & set__amplitude(
    const double & _arg)
  {
    this->amplitude = _arg;
    return *this;
  }
  Type & set__frequency(
    const double & _arg)
  {
    this->frequency = _arg;
    return *this;
  }
  Type & set__velocity_limit(
    const double & _arg)
  {
    this->velocity_limit = _arg;
    return *this;
  }
  Type & set__kp(
    const double & _arg)
  {
    this->kp = _arg;
    return *this;
  }
  Type & set__kd(
    const double & _arg)
  {
    this->kd = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    rs_control::msg::SineWaveParameters_<ContainerAllocator> *;
  using ConstRawPtr =
    const rs_control::msg::SineWaveParameters_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<rs_control::msg::SineWaveParameters_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<rs_control::msg::SineWaveParameters_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      rs_control::msg::SineWaveParameters_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<rs_control::msg::SineWaveParameters_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      rs_control::msg::SineWaveParameters_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<rs_control::msg::SineWaveParameters_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<rs_control::msg::SineWaveParameters_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<rs_control::msg::SineWaveParameters_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__rs_control__msg__SineWaveParameters
    std::shared_ptr<rs_control::msg::SineWaveParameters_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__rs_control__msg__SineWaveParameters
    std::shared_ptr<rs_control::msg::SineWaveParameters_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const SineWaveParameters_ & other) const
  {
    if (this->amplitude != other.amplitude) {
      return false;
    }
    if (this->frequency != other.frequency) {
      return false;
    }
    if (this->velocity_limit != other.velocity_limit) {
      return false;
    }
    if (this->kp != other.kp) {
      return false;
    }
    if (this->kd != other.kd) {
      return false;
    }
    return true;
  }
  bool operator!=(const SineWaveParameters_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct SineWaveParameters_

// alias to use template instance with default allocator
using SineWaveParameters =
  rs_control::msg::SineWaveParameters_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace rs_control

#endif  // RS_CONTROL__MSG__DETAIL__SINE_WAVE_PARAMETERS__STRUCT_HPP_
