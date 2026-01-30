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

#include "nav2_depth_costmap/DepthEstimationNode.hpp"
#include "nav2_depth_costmap/depth_model/DepthAnythingV2.hpp"

#include <cv_bridge/cv_bridge.h>

using std::placeholders::_1;

namespace nav2_depth_costmap
{

DepthEstimationNode::DepthEstimationNode(const rclcpp::NodeOptions & options)
: Node("depth_estimation_node", options),
  camera_info_received_(false) {
  declare_parameters();
  initialize_model();
  initialize_publishers_subscribers();

  RCLCPP_INFO(get_logger(), "DepthEstimationNode initialized");
}

void DepthEstimationNode::declare_parameters() {
  declare_parameter("model_path", "");
  declare_parameter("image_topic", "/camera/image_raw");
  declare_parameter("camera_info_topic", "/camera/camera_info");
  declare_parameter("depth_image_topic", "/depth_estimation/depth");
  declare_parameter("pointcloud_topic", "/depth_estimation/points");
  declare_parameter("model_input_width", 518);
  declare_parameter("model_input_height", 518);
  declare_parameter("min_depth", 0.1);
  declare_parameter("max_depth", 10.0);
  declare_parameter("depth_scale", 10.0);
  declare_parameter("use_gpu", true);
  declare_parameter("publish_depth_image", true);

  model_path_ = get_parameter("model_path").as_string();
  image_topic_ = get_parameter("image_topic").as_string();
  camera_info_topic_ = get_parameter("camera_info_topic").as_string();
  depth_image_topic_ = get_parameter("depth_image_topic").as_string();
  pointcloud_topic_ = get_parameter("pointcloud_topic").as_string();
  model_input_width_ = get_parameter("model_input_width").as_int();
  model_input_height_ = get_parameter("model_input_height").as_int();
  min_depth_ = get_parameter("min_depth").as_double();
  max_depth_ = get_parameter("max_depth").as_double();
  depth_scale_ = get_parameter("depth_scale").as_double();
  use_gpu_ = get_parameter("use_gpu").as_bool();
  publish_depth_image_ = get_parameter("publish_depth_image").as_bool();

  if (model_path_.empty()) {
    RCLCPP_ERROR(get_logger(), "model_path parameter is required");
    throw std::runtime_error("model_path parameter is required");
  }
}

void DepthEstimationNode::initialize_model() {
  // Create depth model
  depth_model_ = std::make_unique<DepthAnythingV2>(
    model_input_width_, model_input_height_, use_gpu_);

  if (!depth_model_->load(model_path_)) {
    RCLCPP_ERROR(get_logger(), "Failed to load model from: %s", model_path_.c_str());
    throw std::runtime_error("Failed to load depth model");
  }

  RCLCPP_INFO(get_logger(), "Loaded depth model: %s", depth_model_->getName().c_str());

  // Create preprocessor and pointcloud generator
  preprocessor_ = std::make_unique<ImagePreprocessor>();
  pointcloud_generator_ = std::make_unique<PointCloudGenerator>(min_depth_, max_depth_);
}

void DepthEstimationNode::initialize_publishers_subscribers() {
  // QoS for sensor data
  rclcpp::QoS sensor_qos = rclcpp::SensorDataQoS();

  // Subscribers
  image_sub_ = create_subscription<sensor_msgs::msg::Image>(
    image_topic_, sensor_qos,
    std::bind(&DepthEstimationNode::image_callback, this, _1));

  camera_info_sub_ = create_subscription<sensor_msgs::msg::CameraInfo>(
    camera_info_topic_, sensor_qos,
    std::bind(&DepthEstimationNode::camera_info_callback, this, _1));

  // Publishers
  if (publish_depth_image_) {
    depth_image_pub_ = create_publisher<sensor_msgs::msg::Image>(
      depth_image_topic_, sensor_qos);
  }

  pointcloud_pub_ = create_publisher<sensor_msgs::msg::PointCloud2>(
    pointcloud_topic_, sensor_qos);

  RCLCPP_INFO(get_logger(), "Subscribing to: %s", image_topic_.c_str());
  RCLCPP_INFO(get_logger(), "Publishing pointcloud to: %s", pointcloud_topic_.c_str());
}

void DepthEstimationNode::camera_info_callback(sensor_msgs::msg::CameraInfo::UniquePtr msg) {
  if (!camera_info_received_) {
    pointcloud_generator_->setCameraInfo(*msg);
    camera_info_received_ = true;
    RCLCPP_INFO(get_logger(),
      "Received camera_info: %dx%d, fx=%.2f, fy=%.2f",
      msg->width, msg->height, msg->k[0], msg->k[4]);
  }
}

void DepthEstimationNode::image_callback(sensor_msgs::msg::Image::UniquePtr msg) {
  if (!camera_info_received_) {
    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000,
      "Waiting for camera_info...");
    return;
  }

  if (!depth_model_->isReady()) {
    RCLCPP_ERROR(get_logger(), "Depth model not ready");
    return;
  }

  try {
    // Convert ROS image to OpenCV
    cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(*msg, "bgr8");
    cv::Mat input_image = cv_ptr->image;

    // Run depth estimation
    cv::Mat relative_depth = depth_model_->infer(input_image);

    // Map relative depth [0,1] to metric depth [min_depth_, max_depth_]
    // relative_depth: 0 = closest, 1 = farthest (after inversion in model)
    cv::Mat metric_depth = min_depth_ + relative_depth * (max_depth_ - min_depth_);

    // Publish depth image if enabled
    if (publish_depth_image_ && depth_image_pub_) {
      cv_bridge::CvImage depth_msg;
      depth_msg.header = msg->header;
      depth_msg.encoding = sensor_msgs::image_encodings::TYPE_32FC1;
      depth_msg.image = metric_depth;
      depth_image_pub_->publish(*depth_msg.toImageMsg());
    }

    // Generate and publish pointcloud
    auto cloud_msg = pointcloud_generator_->generate(metric_depth, msg->header);
    if (cloud_msg) {
      pointcloud_pub_->publish(std::move(cloud_msg));
    }
  } catch (const cv_bridge::Exception & e) {
    RCLCPP_ERROR(get_logger(), "cv_bridge exception: %s", e.what());
  } catch (const std::exception & e) {
    RCLCPP_ERROR(get_logger(), "Error processing image: %s", e.what());
  }
}

}  // namespace nav2_depth_costmap

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(nav2_depth_costmap::DepthEstimationNode)
