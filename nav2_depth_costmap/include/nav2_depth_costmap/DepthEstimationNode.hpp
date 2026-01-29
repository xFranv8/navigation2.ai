// Copyright 2026 Francisco C. Vazquez
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef NAV2_DEPTH_COSTMAP__DEPTHESTIMATIONNODE_HPP_
#define NAV2_DEPTH_COSTMAP__DEPTHESTIMATIONNODE_HPP_

#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "sensor_msgs/msg/camera_info.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"

#include "nav2_depth_costmap/depth_model/DepthModel.hpp"
#include "nav2_depth_costmap/ImagePreprocessor.hpp"
#include "nav2_depth_costmap/PointCloudGenerator.hpp"

namespace nav2_depth_costmap
{

/**
 * @class DepthEstimationNode
 * @brief ROS 2 node for depth estimation and pointcloud generation.
 *
 * This node orchestrates the full pipeline:
 * 1. Receives RGB images from camera
 * 2. Runs depth estimation using a configurable model
 * 3. Converts depth to PointCloud2
 * 4. Publishes PointCloud2 for VoxelLayer consumption
 *
 * The node is model-agnostic - any DepthModel implementation can be used.
 */
class DepthEstimationNode : public rclcpp::Node {
public:
  explicit DepthEstimationNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

protected:
  void image_callback(sensor_msgs::msg::Image::UniquePtr msg);
  void camera_info_callback(sensor_msgs::msg::CameraInfo::UniquePtr msg);

private:
  void declare_parameters();
  void initialize_model();
  void initialize_publishers_subscribers();

  // Components
  std::unique_ptr<DepthModel> depth_model_;
  std::unique_ptr<ImagePreprocessor> preprocessor_;
  std::unique_ptr<PointCloudGenerator> pointcloud_generator_;

  // Subscribers
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Subscription<sensor_msgs::msg::CameraInfo>::SharedPtr camera_info_sub_;

  // Publishers
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr depth_image_pub_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pointcloud_pub_;

  // Parameters
  std::string model_path_;
  std::string image_topic_;
  std::string camera_info_topic_;
  std::string depth_image_topic_;
  std::string pointcloud_topic_;
  int model_input_width_;
  int model_input_height_;
  double min_depth_;
  double max_depth_;
  double depth_scale_;
  bool use_gpu_;
  bool publish_depth_image_;

  bool camera_info_received_;
};

}  // namespace nav2_depth_costmap

#endif  // NAV2_DEPTH_COSTMAP__DEPTHESTIMATIONNODE_HPP_
