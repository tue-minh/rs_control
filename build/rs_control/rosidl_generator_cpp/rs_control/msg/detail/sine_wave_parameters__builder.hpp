// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from rs_control:msg/SineWaveParameters.idl
// generated code does not contain a copyright notice

#ifndef RS_CONTROL__MSG__DETAIL__SINE_WAVE_PARAMETERS__BUILDER_HPP_
#define RS_CONTROL__MSG__DETAIL__SINE_WAVE_PARAMETERS__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "rs_control/msg/detail/sine_wave_parameters__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace rs_control
{

namespace msg
{

namespace builder
{

class Init_SineWaveParameters_kd
{
public:
  explicit Init_SineWaveParameters_kd(::rs_control::msg::SineWaveParameters & msg)
  : msg_(msg)
  {}
  ::rs_control::msg::SineWaveParameters kd(::rs_control::msg::SineWaveParameters::_kd_type arg)
  {
    msg_.kd = std::move(arg);
    return std::move(msg_);
  }

private:
  ::rs_control::msg::SineWaveParameters msg_;
};

class Init_SineWaveParameters_kp
{
public:
  explicit Init_SineWaveParameters_kp(::rs_control::msg::SineWaveParameters & msg)
  : msg_(msg)
  {}
  Init_SineWaveParameters_kd kp(::rs_control::msg::SineWaveParameters::_kp_type arg)
  {
    msg_.kp = std::move(arg);
    return Init_SineWaveParameters_kd(msg_);
  }

private:
  ::rs_control::msg::SineWaveParameters msg_;
};

class Init_SineWaveParameters_velocity_limit
{
public:
  explicit Init_SineWaveParameters_velocity_limit(::rs_control::msg::SineWaveParameters & msg)
  : msg_(msg)
  {}
  Init_SineWaveParameters_kp velocity_limit(::rs_control::msg::SineWaveParameters::_velocity_limit_type arg)
  {
    msg_.velocity_limit = std::move(arg);
    return Init_SineWaveParameters_kp(msg_);
  }

private:
  ::rs_control::msg::SineWaveParameters msg_;
};

class Init_SineWaveParameters_frequency
{
public:
  explicit Init_SineWaveParameters_frequency(::rs_control::msg::SineWaveParameters & msg)
  : msg_(msg)
  {}
  Init_SineWaveParameters_velocity_limit frequency(::rs_control::msg::SineWaveParameters::_frequency_type arg)
  {
    msg_.frequency = std::move(arg);
    return Init_SineWaveParameters_velocity_limit(msg_);
  }

private:
  ::rs_control::msg::SineWaveParameters msg_;
};

class Init_SineWaveParameters_amplitude
{
public:
  Init_SineWaveParameters_amplitude()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_SineWaveParameters_frequency amplitude(::rs_control::msg::SineWaveParameters::_amplitude_type arg)
  {
    msg_.amplitude = std::move(arg);
    return Init_SineWaveParameters_frequency(msg_);
  }

private:
  ::rs_control::msg::SineWaveParameters msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::rs_control::msg::SineWaveParameters>()
{
  return rs_control::msg::builder::Init_SineWaveParameters_amplitude();
}

}  // namespace rs_control

#endif  // RS_CONTROL__MSG__DETAIL__SINE_WAVE_PARAMETERS__BUILDER_HPP_
