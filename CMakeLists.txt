cmake_minimum_required(VERSION 3.8)
project(rs_control)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# Generate messages
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/SineWaveParameters.msg"
  "msg/MotorState.msg"
  DEPENDENCIES std_msgs
)

include_directories(include ${CMAKE_BINARY_DIR}/rosidl_generator_cpp)

add_library(rob_stride_motor src/rob_stride_motor.cpp)
ament_target_dependencies(rob_stride_motor rclcpp)

add_executable(dual_controller_node src/dual_controller_node.cpp)
ament_target_dependencies(dual_controller_node rclcpp std_msgs sensor_msgs rosidl_default_runtime)
target_link_libraries(dual_controller_node rob_stride_motor rs_control__rosidl_typesupport_cpp)

add_executable(full_mit_controller_node src/full_mit_controller_node.cpp)
ament_target_dependencies(full_mit_controller_node rclcpp std_msgs rosidl_default_runtime)
target_link_libraries(full_mit_controller_node rob_stride_motor rs_control__rosidl_typesupport_cpp)

add_executable(haptic_control_node src/haptic_control_node.cpp)
ament_target_dependencies(haptic_control_node rclcpp std_msgs rosidl_default_runtime)
target_link_libraries(haptic_control_node rob_stride_motor rs_control__rosidl_typesupport_cpp)

install(TARGETS rob_stride_motor
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

install(TARGETS dual_controller_node
  DESTINATION lib/${PROJECT_NAME}
)

install(TARGETS full_mit_controller_node
  DESTINATION lib/${PROJECT_NAME}
)

install(TARGETS haptic_control_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/rs_control/
  DESTINATION include/${PROJECT_NAME}
)

install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
