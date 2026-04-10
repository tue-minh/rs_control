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
)

include_directories(include ${CMAKE_BINARY_DIR}/rosidl_generator_cpp)

add_library(rob_stride_motor src/rob_stride_motor.cpp)
ament_target_dependencies(rob_stride_motor rclcpp)
install(TARGETS rob_stride_motor
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

add_executable(main_control_node src/main_control_node.cpp src/can_interface.cpp)
ament_target_dependencies(main_control_node rclcpp std_msgs sensor_msgs)

add_executable(motor_state_reader_node src/motor_state_reader_node.cpp src/can_interface.cpp)
ament_target_dependencies(motor_state_reader_node rclcpp std_msgs sensor_msgs)

add_executable(motor_checker_node src/motor_checker_node.cpp src/can_interface.cpp)
ament_target_dependencies(motor_checker_node rclcpp std_msgs sensor_msgs)

add_executable(dual_motor_control_node src/dual_motor_control_node.cpp)
ament_target_dependencies(dual_motor_control_node rclcpp std_msgs sensor_msgs rosidl_default_runtime)
target_link_libraries(dual_motor_control_node rob_stride_motor rs_control__rosidl_typesupport_cpp)

install(DIRECTORY include/rs_control/
  DESTINATION include/${PROJECT_NAME}
)

install(PROGRAMS
  sine_wave_test.py
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
