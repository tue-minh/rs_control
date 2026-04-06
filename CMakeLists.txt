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

include_directories(include)

add_executable(main_control_node src/main_control_node.cpp src/can_interface.cpp)
ament_target_dependencies(main_control_node rclcpp std_msgs sensor_msgs)

add_executable(motor_state_reader_node src/motor_state_reader_node.cpp src/can_interface.cpp)
ament_target_dependencies(motor_state_reader_node rclcpp std_msgs sensor_msgs)

add_executable(motor_checker_node src/motor_checker_node.cpp src/can_interface.cpp)
ament_target_dependencies(motor_checker_node rclcpp std_msgs sensor_msgs)

install(TARGETS
  main_control_node
  motor_state_reader_node
  motor_checker_node
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
