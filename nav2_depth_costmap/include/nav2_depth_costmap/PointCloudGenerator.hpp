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

#ifndef NAV2_DEPTH_COSTMAP__POINTCLOUDGENERATOR_HPP_
#define NAV2_DEPTH_COSTMAP__POINTCLOUDGENERATOR_HPP_

#include <memory>

#include <opencv2/opencv.hpp>

#include "sensor_msgs/msg/point_cloud2.hpp"
#include "sensor_msgs/msg/camera_info.hpp"
#include "std_msgs/msg/header.hpp"

namespace nav2_depth_costmap
{

/**
 * @struct CameraIntrinsics
 * @brief Camera intrinsic parameters for 3D projection
 */
struct CameraIntrinsics {
  double fx;  // Focal length x
  double fy;  // Focal length y
  double cx;  // Principal point x
  double cy;  // Principal point y
  int width;
  int height;
};

/**
 * @class PointCloudGenerator
 * @brief Converts depth images to PointCloud2 messages.
 *
 * Uses camera intrinsics to back-project depth values to 3D points.
 * Applies depth filtering to exclude points outside valid range.
 */
class PointCloudGenerator {
public:
  /**
   * @brief Constructor
   * @param min_depth Minimum valid depth in meters
   * @param max_depth Maximum valid depth in meters
   */
  explicit PointCloudGenerator(double min_depth = 0.1, double max_depth = 10.0);

  /**
   * @brief Set camera intrinsics from CameraInfo message
   * @param camera_info ROS CameraInfo message
   */
  void setCameraInfo(const sensor_msgs::msg::CameraInfo & camera_info);

  /**
   * @brief Set camera intrinsics directly
   * @param intrinsics Camera intrinsic parameters
   */
  void setCameraIntrinsics(const CameraIntrinsics & intrinsics);

  /**
   * @brief Update intrinsics for a resized image
   * @param new_width New image width
   * @param new_height New image height
   */
  void updateIntrinsicsForSize(int new_width, int new_height);

  /**
   * @brief Check if camera intrinsics have been set
   * @return true if intrinsics are valid
   */
  bool hasValidIntrinsics() const;

  /**
   * @brief Convert depth image to PointCloud2
   * @param depth_image Depth image (CV_32FC1) in meters
   * @param header ROS message header for the output
   * @return PointCloud2 message, or nullptr if conversion fails
   */
  sensor_msgs::msg::PointCloud2::UniquePtr generate(
    const cv::Mat & depth_image,
    const std_msgs::msg::Header & header);

  /**
   * @brief Set depth range for filtering
   * @param min_depth Minimum valid depth
   * @param max_depth Maximum valid depth
   */
  void setDepthRange(double min_depth, double max_depth);

  /**
   * @brief Get current minimum depth
   * @return Minimum depth in meters
   */
  double getMinDepth() const { return min_depth_; }

  /**
   * @brief Get current maximum depth
   * @return Maximum depth in meters
   */
  double getMaxDepth() const { return max_depth_; }

private:
  CameraIntrinsics intrinsics_;
  CameraIntrinsics original_intrinsics_;
  double min_depth_;
  double max_depth_;
  bool has_intrinsics_;
};

}  // namespace nav2_depth_costmap

#endif  // NAV2_DEPTH_COSTMAP__POINTCLOUDGENERATOR_HPP_
