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

#ifndef NAV2_DEPTH_COSTMAP__IMAGEPREPROCESSOR_HPP_
#define NAV2_DEPTH_COSTMAP__IMAGEPREPROCESSOR_HPP_

#include <opencv2/opencv.hpp>

namespace nav2_depth_costmap
{

/**
 * @class ImagePreprocessor
 * @brief Handles image preprocessing operations for the depth estimation pipeline.
 *
 * Responsibilities:
 * - Downsampling/resizing images
 * - Color space conversions
 * - Image normalization
 */
class ImagePreprocessor {
public:
  /**
   * @brief Constructor
   * @param target_width Target width for downsampling (0 = no resize)
   * @param target_height Target height for downsampling (0 = no resize)
   */
  explicit ImagePreprocessor(int target_width = 0, int target_height = 0);

  /**
   * @brief Resize image to target dimensions
   * @param input Input image
   * @return Resized image (or original if target is 0)
   */
  cv::Mat resize(const cv::Mat & input) const;

  /**
   * @brief Resize image to specific dimensions
   * @param input Input image
   * @param width Target width
   * @param height Target height
   * @return Resized image
   */
  static cv::Mat resize(const cv::Mat & input, int width, int height);

  /**
   * @brief Convert BGR to RGB
   * @param bgr_image Input BGR image
   * @return RGB image
   */
  static cv::Mat bgrToRgb(const cv::Mat & bgr_image);

  /**
   * @brief Convert RGB to BGR
   * @param rgb_image Input RGB image
   * @return BGR image
   */
  static cv::Mat rgbToBgr(const cv::Mat & rgb_image);

  /**
   * @brief Set target dimensions for resizing
   * @param width Target width
   * @param height Target height
   */
  void setTargetSize(int width, int height);

  /**
   * @brief Get current target width
   * @return Target width
   */
  int getTargetWidth() const { return target_width_; }

  /**
   * @brief Get current target height
   * @return Target height
   */
  int getTargetHeight() const { return target_height_; }

private:
  int target_width_;
  int target_height_;
};

}  // namespace nav2_depth_costmap

#endif  // NAV2_DEPTH_COSTMAP__IMAGEPREPROCESSOR_HPP_
