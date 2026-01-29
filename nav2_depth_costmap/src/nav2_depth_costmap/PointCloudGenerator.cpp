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

#include "nav2_depth_costmap/PointCloudGenerator.hpp"

#include <cmath>
#include <sensor_msgs/point_cloud2_iterator.hpp>

namespace nav2_depth_costmap
{

PointCloudGenerator::PointCloudGenerator(double min_depth, double max_depth)
: min_depth_(min_depth),
  max_depth_(max_depth),
  has_intrinsics_(false) {
  intrinsics_ = {0.0, 0.0, 0.0, 0.0, 0, 0};
  original_intrinsics_ = intrinsics_;
}

void PointCloudGenerator::setCameraInfo(const sensor_msgs::msg::CameraInfo & camera_info) {
  intrinsics_.fx = camera_info.k[0];
  intrinsics_.fy = camera_info.k[4];
  intrinsics_.cx = camera_info.k[2];
  intrinsics_.cy = camera_info.k[5];
  intrinsics_.width = static_cast<int>(camera_info.width);
  intrinsics_.height = static_cast<int>(camera_info.height);
  original_intrinsics_ = intrinsics_;
  has_intrinsics_ = (intrinsics_.fx > 0 && intrinsics_.fy > 0);
}

void PointCloudGenerator::setCameraIntrinsics(const CameraIntrinsics & intrinsics) {
  intrinsics_ = intrinsics;
  original_intrinsics_ = intrinsics;
  has_intrinsics_ = (intrinsics_.fx > 0 && intrinsics_.fy > 0);
}

void PointCloudGenerator::updateIntrinsicsForSize(int new_width, int new_height) {
  if (original_intrinsics_.width <= 0 || original_intrinsics_.height <= 0) {
    return;
  }

  double scale_x = static_cast<double>(new_width) / original_intrinsics_.width;
  double scale_y = static_cast<double>(new_height) / original_intrinsics_.height;

  intrinsics_.fx = original_intrinsics_.fx * scale_x;
  intrinsics_.fy = original_intrinsics_.fy * scale_y;
  intrinsics_.cx = original_intrinsics_.cx * scale_x;
  intrinsics_.cy = original_intrinsics_.cy * scale_y;
  intrinsics_.width = new_width;
  intrinsics_.height = new_height;
}

bool PointCloudGenerator::hasValidIntrinsics() const {
  return has_intrinsics_;
}

sensor_msgs::msg::PointCloud2::UniquePtr PointCloudGenerator::generate(
  const cv::Mat & depth_image,
  const std_msgs::msg::Header & header) {
  if (!has_intrinsics_) {
    return nullptr;
  }

  if (depth_image.type() != CV_32FC1) {
    return nullptr;
  }

  // Update intrinsics if depth image size differs from original
  if (depth_image.cols != intrinsics_.width || depth_image.rows != intrinsics_.height) {
    updateIntrinsicsForSize(depth_image.cols, depth_image.rows);
  }

  const double fx = intrinsics_.fx;
  const double fy = intrinsics_.fy;
  const double cx = intrinsics_.cx;
  const double cy = intrinsics_.cy;

  // Count valid points for efficient memory allocation
  int valid_points = 0;
  for (int v = 0; v < depth_image.rows; ++v) {
    const float * row_ptr = depth_image.ptr<float>(v);
    for (int u = 0; u < depth_image.cols; ++u) {
      float depth = row_ptr[u];
      if (std::isfinite(depth) && depth >= min_depth_ && depth <= max_depth_) {
        ++valid_points;
      }
    }
  }

  if (valid_points == 0) {
    return nullptr;
  }

  // Create PointCloud2 message
  auto cloud_msg = std::make_unique<sensor_msgs::msg::PointCloud2>();
  cloud_msg->header = header;
  cloud_msg->height = 1;
  cloud_msg->width = valid_points;
  cloud_msg->is_dense = true;
  cloud_msg->is_bigendian = false;

  // Define point fields (x, y, z)
  sensor_msgs::PointCloud2Modifier modifier(*cloud_msg);
  modifier.setPointCloud2FieldsByString(1, "xyz");
  modifier.resize(valid_points);

  // Fill point cloud using iterators
  sensor_msgs::PointCloud2Iterator<float> iter_x(*cloud_msg, "x");
  sensor_msgs::PointCloud2Iterator<float> iter_y(*cloud_msg, "y");
  sensor_msgs::PointCloud2Iterator<float> iter_z(*cloud_msg, "z");

  for (int v = 0; v < depth_image.rows; ++v) {
    const float * row_ptr = depth_image.ptr<float>(v);
    for (int u = 0; u < depth_image.cols; ++u) {
      float depth = row_ptr[u];

      if (std::isfinite(depth) && depth >= min_depth_ && depth <= max_depth_) {
        // Back-project to 3D using pinhole camera model
        *iter_x = static_cast<float>((u - cx) * depth / fx);
        *iter_y = static_cast<float>((v - cy) * depth / fy);
        *iter_z = depth;

        ++iter_x;
        ++iter_y;
        ++iter_z;
      }
    }
  }

  return cloud_msg;
}

void PointCloudGenerator::setDepthRange(double min_depth, double max_depth) {
  min_depth_ = min_depth;
  max_depth_ = max_depth;
}

}  // namespace nav2_depth_costmap
