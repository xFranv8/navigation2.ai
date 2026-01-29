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

#include "nav2_depth_costmap/ImagePreprocessor.hpp"

namespace nav2_depth_costmap
{

ImagePreprocessor::ImagePreprocessor(int target_width, int target_height)
: target_width_(target_width),
  target_height_(target_height) {
}

cv::Mat ImagePreprocessor::resize(const cv::Mat & input) const {
  if (target_width_ <= 0 || target_height_ <= 0) {
    return input.clone();
  }
  return resize(input, target_width_, target_height_);
}

cv::Mat ImagePreprocessor::resize(const cv::Mat & input, int width, int height) {
  cv::Mat output;
  cv::resize(input, output, cv::Size(width, height), 0, 0, cv::INTER_AREA);
  return output;
}

cv::Mat ImagePreprocessor::bgrToRgb(const cv::Mat & bgr_image) {
  cv::Mat rgb;
  cv::cvtColor(bgr_image, rgb, cv::COLOR_BGR2RGB);
  return rgb;
}

cv::Mat ImagePreprocessor::rgbToBgr(const cv::Mat & rgb_image) {
  cv::Mat bgr;
  cv::cvtColor(rgb_image, bgr, cv::COLOR_RGB2BGR);
  return bgr;
}

void ImagePreprocessor::setTargetSize(int width, int height) {
  target_width_ = width;
  target_height_ = height;
}

}  // namespace nav2_depth_costmap
