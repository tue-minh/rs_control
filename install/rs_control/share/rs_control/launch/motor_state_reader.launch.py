from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='rs_control',
            executable='motor_state_reader_node',
            name='motor_state_reader_node',
            output='screen',
            parameters=[{
                'motor_id': 127,
                'can_interface': 'can0',
                'read_frequency': 50.0,
            }]
        ),
    ])
