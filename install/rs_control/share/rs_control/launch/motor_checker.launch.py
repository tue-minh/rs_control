"""Launch file for motor checker node."""

from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='rs_control',
            executable='motor_checker_node',
            name='motor_checker_node',
            output='screen',
            parameters=[{
                'expected_motor_id': 127,
                'can_interface': 'can0',
                'timeout_ms': 500,
                'auto_check': True,
                'check_interval': 5.0,
            }],
        ),
    ])
