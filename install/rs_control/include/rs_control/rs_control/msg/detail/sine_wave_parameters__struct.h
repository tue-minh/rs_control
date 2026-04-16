// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from rs_control:msg/SineWaveParameters.idl
// generated code does not contain a copyright notice

#ifndef RS_CONTROL__MSG__DETAIL__SINE_WAVE_PARAMETERS__STRUCT_H_
#define RS_CONTROL__MSG__DETAIL__SINE_WAVE_PARAMETERS__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in msg/SineWaveParameters in the package rs_control.
/**
  * SineWaveParameters.msg
  * Message for sine wave motion parameters
 */
typedef struct rs_control__msg__SineWaveParameters
{
  /// Amplitude of sine wave (radians)
  double amplitude;
  /// Frequency of sine wave (Hz)
  double frequency;
  /// Maximum velocity limit (rad/s)
  double velocity_limit;
  /// Position gain
  double kp;
  /// Velocity gain
  double kd;
} rs_control__msg__SineWaveParameters;

// Struct for a sequence of rs_control__msg__SineWaveParameters.
typedef struct rs_control__msg__SineWaveParameters__Sequence
{
  rs_control__msg__SineWaveParameters * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} rs_control__msg__SineWaveParameters__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // RS_CONTROL__MSG__DETAIL__SINE_WAVE_PARAMETERS__STRUCT_H_
