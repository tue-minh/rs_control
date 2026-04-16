// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from rs_control:msg/SineWaveParameters.idl
// generated code does not contain a copyright notice
#include "rs_control/msg/detail/sine_wave_parameters__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


bool
rs_control__msg__SineWaveParameters__init(rs_control__msg__SineWaveParameters * msg)
{
  if (!msg) {
    return false;
  }
  // amplitude
  // frequency
  // velocity_limit
  // kp
  // kd
  return true;
}

void
rs_control__msg__SineWaveParameters__fini(rs_control__msg__SineWaveParameters * msg)
{
  if (!msg) {
    return;
  }
  // amplitude
  // frequency
  // velocity_limit
  // kp
  // kd
}

bool
rs_control__msg__SineWaveParameters__are_equal(const rs_control__msg__SineWaveParameters * lhs, const rs_control__msg__SineWaveParameters * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // amplitude
  if (lhs->amplitude != rhs->amplitude) {
    return false;
  }
  // frequency
  if (lhs->frequency != rhs->frequency) {
    return false;
  }
  // velocity_limit
  if (lhs->velocity_limit != rhs->velocity_limit) {
    return false;
  }
  // kp
  if (lhs->kp != rhs->kp) {
    return false;
  }
  // kd
  if (lhs->kd != rhs->kd) {
    return false;
  }
  return true;
}

bool
rs_control__msg__SineWaveParameters__copy(
  const rs_control__msg__SineWaveParameters * input,
  rs_control__msg__SineWaveParameters * output)
{
  if (!input || !output) {
    return false;
  }
  // amplitude
  output->amplitude = input->amplitude;
  // frequency
  output->frequency = input->frequency;
  // velocity_limit
  output->velocity_limit = input->velocity_limit;
  // kp
  output->kp = input->kp;
  // kd
  output->kd = input->kd;
  return true;
}

rs_control__msg__SineWaveParameters *
rs_control__msg__SineWaveParameters__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  rs_control__msg__SineWaveParameters * msg = (rs_control__msg__SineWaveParameters *)allocator.allocate(sizeof(rs_control__msg__SineWaveParameters), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(rs_control__msg__SineWaveParameters));
  bool success = rs_control__msg__SineWaveParameters__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
rs_control__msg__SineWaveParameters__destroy(rs_control__msg__SineWaveParameters * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    rs_control__msg__SineWaveParameters__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
rs_control__msg__SineWaveParameters__Sequence__init(rs_control__msg__SineWaveParameters__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  rs_control__msg__SineWaveParameters * data = NULL;

  if (size) {
    data = (rs_control__msg__SineWaveParameters *)allocator.zero_allocate(size, sizeof(rs_control__msg__SineWaveParameters), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = rs_control__msg__SineWaveParameters__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        rs_control__msg__SineWaveParameters__fini(&data[i - 1]);
      }
      allocator.deallocate(data, allocator.state);
      return false;
    }
  }
  array->data = data;
  array->size = size;
  array->capacity = size;
  return true;
}

void
rs_control__msg__SineWaveParameters__Sequence__fini(rs_control__msg__SineWaveParameters__Sequence * array)
{
  if (!array) {
    return;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  if (array->data) {
    // ensure that data and capacity values are consistent
    assert(array->capacity > 0);
    // finalize all array elements
    for (size_t i = 0; i < array->capacity; ++i) {
      rs_control__msg__SineWaveParameters__fini(&array->data[i]);
    }
    allocator.deallocate(array->data, allocator.state);
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
  } else {
    // ensure that data, size, and capacity values are consistent
    assert(0 == array->size);
    assert(0 == array->capacity);
  }
}

rs_control__msg__SineWaveParameters__Sequence *
rs_control__msg__SineWaveParameters__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  rs_control__msg__SineWaveParameters__Sequence * array = (rs_control__msg__SineWaveParameters__Sequence *)allocator.allocate(sizeof(rs_control__msg__SineWaveParameters__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = rs_control__msg__SineWaveParameters__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
rs_control__msg__SineWaveParameters__Sequence__destroy(rs_control__msg__SineWaveParameters__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    rs_control__msg__SineWaveParameters__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
rs_control__msg__SineWaveParameters__Sequence__are_equal(const rs_control__msg__SineWaveParameters__Sequence * lhs, const rs_control__msg__SineWaveParameters__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!rs_control__msg__SineWaveParameters__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
rs_control__msg__SineWaveParameters__Sequence__copy(
  const rs_control__msg__SineWaveParameters__Sequence * input,
  rs_control__msg__SineWaveParameters__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(rs_control__msg__SineWaveParameters);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    rs_control__msg__SineWaveParameters * data =
      (rs_control__msg__SineWaveParameters *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!rs_control__msg__SineWaveParameters__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          rs_control__msg__SineWaveParameters__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!rs_control__msg__SineWaveParameters__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
