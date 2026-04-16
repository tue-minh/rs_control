// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from rs_control:msg/SineWaveParameters.idl
// generated code does not contain a copyright notice

#ifndef RS_CONTROL__MSG__DETAIL__SINE_WAVE_PARAMETERS__FUNCTIONS_H_
#define RS_CONTROL__MSG__DETAIL__SINE_WAVE_PARAMETERS__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/visibility_control.h"
#include "rs_control/msg/rosidl_generator_c__visibility_control.h"

#include "rs_control/msg/detail/sine_wave_parameters__struct.h"

/// Initialize msg/SineWaveParameters message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * rs_control__msg__SineWaveParameters
 * )) before or use
 * rs_control__msg__SineWaveParameters__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_rs_control
bool
rs_control__msg__SineWaveParameters__init(rs_control__msg__SineWaveParameters * msg);

/// Finalize msg/SineWaveParameters message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_rs_control
void
rs_control__msg__SineWaveParameters__fini(rs_control__msg__SineWaveParameters * msg);

/// Create msg/SineWaveParameters message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * rs_control__msg__SineWaveParameters__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_rs_control
rs_control__msg__SineWaveParameters *
rs_control__msg__SineWaveParameters__create();

/// Destroy msg/SineWaveParameters message.
/**
 * It calls
 * rs_control__msg__SineWaveParameters__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_rs_control
void
rs_control__msg__SineWaveParameters__destroy(rs_control__msg__SineWaveParameters * msg);

/// Check for msg/SineWaveParameters message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_rs_control
bool
rs_control__msg__SineWaveParameters__are_equal(const rs_control__msg__SineWaveParameters * lhs, const rs_control__msg__SineWaveParameters * rhs);

/// Copy a msg/SineWaveParameters message.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source message pointer.
 * \param[out] output The target message pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer is null
 *   or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_rs_control
bool
rs_control__msg__SineWaveParameters__copy(
  const rs_control__msg__SineWaveParameters * input,
  rs_control__msg__SineWaveParameters * output);

/// Initialize array of msg/SineWaveParameters messages.
/**
 * It allocates the memory for the number of elements and calls
 * rs_control__msg__SineWaveParameters__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_rs_control
bool
rs_control__msg__SineWaveParameters__Sequence__init(rs_control__msg__SineWaveParameters__Sequence * array, size_t size);

/// Finalize array of msg/SineWaveParameters messages.
/**
 * It calls
 * rs_control__msg__SineWaveParameters__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_rs_control
void
rs_control__msg__SineWaveParameters__Sequence__fini(rs_control__msg__SineWaveParameters__Sequence * array);

/// Create array of msg/SineWaveParameters messages.
/**
 * It allocates the memory for the array and calls
 * rs_control__msg__SineWaveParameters__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_rs_control
rs_control__msg__SineWaveParameters__Sequence *
rs_control__msg__SineWaveParameters__Sequence__create(size_t size);

/// Destroy array of msg/SineWaveParameters messages.
/**
 * It calls
 * rs_control__msg__SineWaveParameters__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_rs_control
void
rs_control__msg__SineWaveParameters__Sequence__destroy(rs_control__msg__SineWaveParameters__Sequence * array);

/// Check for msg/SineWaveParameters message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_rs_control
bool
rs_control__msg__SineWaveParameters__Sequence__are_equal(const rs_control__msg__SineWaveParameters__Sequence * lhs, const rs_control__msg__SineWaveParameters__Sequence * rhs);

/// Copy an array of msg/SineWaveParameters messages.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source array pointer.
 * \param[out] output The target array pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer
 *   is null or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_rs_control
bool
rs_control__msg__SineWaveParameters__Sequence__copy(
  const rs_control__msg__SineWaveParameters__Sequence * input,
  rs_control__msg__SineWaveParameters__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // RS_CONTROL__MSG__DETAIL__SINE_WAVE_PARAMETERS__FUNCTIONS_H_
