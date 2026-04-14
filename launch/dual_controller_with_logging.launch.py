"""Launch file for dual controller node with motor logging and Foxglove bridge.

This launch file:
1. Starts the dual_controller_node for controlling 2 RobStride motors
2. Logs all motor state and command topics using ros2 topic echo
3. Starts a Foxglove Bridge for visualization in Foxglove Studio
"""

from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess, RegisterEventHandler, EmitEvent
from launch.events import Shutdown
from launch.event_handlers import OnProcessExit


def generate_launch_description():
    return LaunchDescription([
        # 1. Dual Controller Node
        Node(
            package='rs_control',
            executable='dual_controller_node',
            name='dual_controller_node',
            output='screen',
            parameters=[{
                'motor1_id': 1,
                'motor2_id': 127,
                'can_interface': 'can0',
            }],
            emulate_tty=True,
        ),

        # 2. Log Motor 1 State
        ExecuteProcess(
            cmd=['ros2', 'topic', 'echo', '/motor1/state', 'sensor_msgs/msg/JointState'],
            output='screen',
            name='echo_motor1_state',
        ),

        # 3. Log Motor 2 State
        ExecuteProcess(
            cmd=['ros2', 'topic', 'echo', '/motor2/state', 'sensor_msgs/msg/JointState'],
            output='screen',
            name='echo_motor2_state',
        ),

        # 4. Log Motor 1 Position Commands
        ExecuteProcess(
            cmd=['ros2', 'topic', 'echo', '/motor1/position', 'std_msgs/msg/Float64'],
            output='screen',
            name='echo_motor1_position',
        ),

        # 5. Log Motor 2 Position Commands
        ExecuteProcess(
            cmd=['ros2', 'topic', 'echo', '/motor2/position', 'std_msgs/msg/Float64'],
            output='screen',
            name='echo_motor2_position',
        ),

        # 6. Foxglove Bridge
        Node(
            package='foxglove_bridge',
            executable='foxglove_bridge_node',
            name='foxglove_bridge',
            output='screen',
            parameters=[{
                'port': 8765,
                'address': '0.0.0.0',
                'max_qos_depth': 10,
            }],
            emulate_tty=True,
        ),
    ])
