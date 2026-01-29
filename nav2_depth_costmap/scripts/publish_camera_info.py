#!/usr/bin/env python3
"""Publish fake camera info and TF for testing with image_tools cam2image."""

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import CameraInfo
from geometry_msgs.msg import TransformStamped
from tf2_ros import StaticTransformBroadcaster


class CameraInfoPublisher(Node):
    def __init__(self):
        super().__init__('camera_info_publisher')
        self.publisher = self.create_publisher(CameraInfo, '/camera/camera_info', 10)
        self.timer = self.create_timer(0.1, self.publish_camera_info)  # 10 Hz
        
        # Publish static TF for camera_frame
        self.tf_broadcaster = StaticTransformBroadcaster(self)
        self.publish_static_tf()
        
        self.get_logger().info('Publishing camera_info to /camera/camera_info')
        self.get_logger().info('Publishing TF: map -> camera_frame')

    def publish_static_tf(self):
        t = TransformStamped()
        t.header.stamp = self.get_clock().now().to_msg()
        t.header.frame_id = 'map'
        t.child_frame_id = 'default_cam'
        
        # Position camera at origin, looking forward
        t.transform.translation.x = 0.0
        t.transform.translation.y = 0.0
        t.transform.translation.z = 1.0  # 1 meter height
        
        # No rotation (identity quaternion)
        t.transform.rotation.x = 0.0
        t.transform.rotation.y = 0.0
        t.transform.rotation.z = 0.0
        t.transform.rotation.w = 1.0
        
        self.tf_broadcaster.sendTransform(t)

    def publish_camera_info(self):
        msg = CameraInfo()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = 'camera_frame'
        
        # Image size (burger image from cam2image is 320x240)
        msg.width = 320
        msg.height = 240
        
        # Distortion model
        msg.distortion_model = 'plumb_bob'
        msg.d = [0.0, 0.0, 0.0, 0.0, 0.0]
        
        # Intrinsic camera matrix (K)
        # fx, fy: focal length, cx, cy: principal point
        fx = 277.0  # Approximate for 60 degree FOV
        fy = 277.0
        cx = 160.0  # Image center
        cy = 120.0
        msg.k = [fx, 0.0, cx, 0.0, fy, cy, 0.0, 0.0, 1.0]
        
        # Rectification matrix (identity for monocular)
        msg.r = [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]
        
        # Projection matrix (P)
        msg.p = [fx, 0.0, cx, 0.0, 0.0, fy, cy, 0.0, 0.0, 0.0, 1.0, 0.0]
        
        self.publisher.publish(msg)


def main():
    rclpy.init()
    node = CameraInfoPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
