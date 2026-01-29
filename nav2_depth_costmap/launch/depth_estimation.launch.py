# Copyright 2026 xfranv8
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Launch file for the depth estimation node.

Usage:
    ros2 launch nav2_depth_costmap depth_estimation.launch.py \
        model_path:=/path/to/model.onnx

    # With custom parameters
    ros2 launch nav2_depth_costmap depth_estimation.launch.py \
        model_path:=/path/to/model.onnx \
        image_topic:=/camera/rgb/image_raw
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    # Declare arguments
    model_path_arg = DeclareLaunchArgument(
        'model_path',
        description='Path to ONNX model file (required)'
    )

    params_file_arg = DeclareLaunchArgument(
        'params_file',
        default_value=PathJoinSubstitution([
            FindPackageShare('nav2_depth_costmap'),
            'config',
            'depth_estimation_params.yaml'
        ]),
        description='Path to parameter file'
    )

    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='Use simulation time'
    )

    image_topic_arg = DeclareLaunchArgument(
        'image_topic',
        default_value='/camera/image_raw',
        description='Input RGB image topic'
    )

    camera_info_topic_arg = DeclareLaunchArgument(
        'camera_info_topic',
        default_value='/camera/camera_info',
        description='Camera info topic'
    )

    pointcloud_topic_arg = DeclareLaunchArgument(
        'pointcloud_topic',
        default_value='/depth_estimation/points',
        description='Output pointcloud topic'
    )

    # Get launch configurations
    model_path = LaunchConfiguration('model_path')
    params_file = LaunchConfiguration('params_file')
    use_sim_time = LaunchConfiguration('use_sim_time')
    image_topic = LaunchConfiguration('image_topic')
    camera_info_topic = LaunchConfiguration('camera_info_topic')
    pointcloud_topic = LaunchConfiguration('pointcloud_topic')

    # Node
    depth_estimation_node = Node(
        package='nav2_depth_costmap',
        executable='depth_estimation_node',
        name='depth_estimation_node',
        output='screen',
        parameters=[
            params_file,
            {
                'use_sim_time': use_sim_time,
                'model_path': model_path,
                'image_topic': image_topic,
                'camera_info_topic': camera_info_topic,
                'pointcloud_topic': pointcloud_topic,
            }
        ],
    )

    return LaunchDescription([
        model_path_arg,
        params_file_arg,
        use_sim_time_arg,
        image_topic_arg,
        camera_info_topic_arg,
        pointcloud_topic_arg,
        depth_estimation_node,
    ])
