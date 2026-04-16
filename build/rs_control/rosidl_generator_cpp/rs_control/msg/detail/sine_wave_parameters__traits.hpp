// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from rs_control:msg/SineWaveParameters.idl
// generated code does not contain a copyright notice

#ifndef RS_CONTROL__MSG__DETAIL__SINE_WAVE_PARAMETERS__TRAITS_HPP_
#define RS_CONTROL__MSG__DETAIL__SINE_WAVE_PARAMETERS__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "rs_control/msg/detail/sine_wave_parameters__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace rs_control
{

namespace msg
{

inline void to_flow_style_yaml(
  const SineWaveParameters & msg,
  std::ostream & out)
{
  out << "{";
  // member: amplitude
  {
    out << "amplitude: ";
    rosidl_generator_traits::value_to_yaml(msg.amplitude, out);
    out << ", ";
  }

  // member: frequency
  {
    out << "frequency: ";
    rosidl_generator_traits::value_to_yaml(msg.frequency, out);
    out << ", ";
  }

  // member: velocity_limit
  {
    out << "velocity_limit: ";
    rosidl_generator_traits::value_to_yaml(msg.velocity_limit, out);
    out << ", ";
  }

  // member: kp
  {
    out << "kp: ";
    rosidl_generator_traits::value_to_yaml(msg.kp, out);
    out << ", ";
  }

  // member: kd
  {
    out << "kd: ";
    rosidl_generator_traits::value_to_yaml(msg.kd, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const SineWaveParameters & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: amplitude
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "amplitude: ";
    rosidl_generator_traits::value_to_yaml(msg.amplitude, out);
    out << "\n";
  }

  // member: frequency
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "frequency: ";
    rosidl_generator_traits::value_to_yaml(msg.frequency, out);
    out << "\n";
  }

  // member: velocity_limit
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "velocity_limit: ";
    rosidl_generator_traits::value_to_yaml(msg.velocity_limit, out);
    out << "\n";
  }

  // member: kp
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "kp: ";
    rosidl_generator_traits::value_to_yaml(msg.kp, out);
    out << "\n";
  }

  // member: kd
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "kd: ";
    rosidl_generator_traits::value_to_yaml(msg.kd, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const SineWaveParameters & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace msg

}  // namespace rs_control

namespace rosidl_generator_traits
{

[[deprecated("use rs_control::msg::to_block_style_yaml() instead")]]
inline void to_yaml(
  const rs_control::msg::SineWaveParameters & msg,
  std::ostream & out, size_t indentation = 0)
{
  rs_control::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use rs_control::msg::to_yaml() instead")]]
inline std::string to_yaml(const rs_control::msg::SineWaveParameters & msg)
{
  return rs_control::msg::to_yaml(msg);
}

template<>
inline const char * data_type<rs_control::msg::SineWaveParameters>()
{
  return "rs_control::msg::SineWaveParameters";
}

template<>
inline const char * name<rs_control::msg::SineWaveParameters>()
{
  return "rs_control/msg/SineWaveParameters";
}

template<>
struct has_fixed_size<rs_control::msg::SineWaveParameters>
  : std::integral_constant<bool, true> {};

template<>
struct has_bounded_size<rs_control::msg::SineWaveParameters>
  : std::integral_constant<bool, true> {};

template<>
struct is_message<rs_control::msg::SineWaveParameters>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // RS_CONTROL__MSG__DETAIL__SINE_WAVE_PARAMETERS__TRAITS_HPP_
