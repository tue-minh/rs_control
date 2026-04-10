#!/usr/bin/env python3
"""
Sine Wave Test Script for Dual Motor Control Node

This script demonstrates how to use the sine wave functionality
of the dual_motor_control_node.
"""

import rclpy
from rclpy.node import Node
from rs_control.msg import SineWaveParameters
import time

class SineWaveTester(Node):
    def __init__(self):
        super().__init__('sine_wave_tester')

        # Publishers for sine wave parameters
        self.rs03_publisher = self.create_publisher(
            SineWaveParameters,
            'sine_wave_rs03',
            10
        )

        self.rs06_publisher = self.create_publisher(
            SineWaveParameters,
            'sine_wave_rs06',
            10
        )

        self.get_logger().info('Sine Wave Tester initialized')

    def test_sine_wave(self):
        """Test sine wave motion for both motors"""

        # Test parameters for RS-03
        rs03_params = SineWaveParameters()
        rs03_params.amplitude = 1.0      # 1 radian amplitude
        rs03_params.frequency = 0.5      # 0.5 Hz
        rs03_params.velocity_limit = 5.0 # 5 rad/s max velocity
        rs03_params.kp = 2.0             # Position gain
        rs03_params.kd = 0.1             # Velocity gain

        # Test parameters for RS-06 (different phase/frequency)
        rs06_params = SineWaveParameters()
        rs06_params.amplitude = 0.8      # 0.8 radian amplitude
        rs06_params.frequency = 0.3      # 0.3 Hz
        rs06_params.velocity_limit = 4.0 # 4 rad/s max velocity
        rs06_params.kp = 1.5             # Position gain
        rs06_params.kd = 0.05            # Velocity gain

        self.get_logger().info('Starting sine wave test...')

        # Publish parameters (this enables sine wave mode)
        self.rs03_publisher.publish(rs03_params)
        self.rs06_publisher.publish(rs06_params)

        self.get_logger().info('Sine wave parameters published. Motors should start moving.')
        self.get_logger().info('RS-03: amp=1.0, freq=0.5Hz, kp=2.0, kd=0.1')
        self.get_logger().info('RS-06: amp=0.8, freq=0.3Hz, kp=1.5, kd=0.05')

        # Let it run for 10 seconds
        time.sleep(10)

        self.get_logger().info('Test completed. Motors will continue sine wave motion until new commands.')

def main(args=None):
    rclpy.init(args=args)

    tester = SineWaveTester()

    try:
        tester.test_sine_wave()
        rclpy.spin(tester)
    except KeyboardInterrupt:
        pass
    finally:
        tester.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()