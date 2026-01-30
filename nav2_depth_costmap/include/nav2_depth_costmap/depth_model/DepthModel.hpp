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

#ifndef NAV2_DEPTH_COSTMAP__DEPTH_MODEL__DEPTHMODEL_HPP_
#define NAV2_DEPTH_COSTMAP__DEPTH_MODEL__DEPTHMODEL_HPP_

#include <string>

#include <opencv2/opencv.hpp>

namespace nav2_depth_costmap
{

/**
 * @class DepthModel
 * @brief Abstract interface for depth estimation models.
 *
 * This interface allows the pipeline to work with any depth estimation model
 * (Depth Anything V2, MiDaS, etc.) by implementing this interface.
 * Follows the Strategy pattern for model interchangeability.
 */
class DepthModel {
public:
  virtual ~DepthModel() = default;

  /**
   * @brief Load the model from file
   * @param model_path Path to the model file
   * @return true if loading succeeded
   */
  virtual bool load(const std::string & model_path) = 0;

  /**
   * @brief Check if model is loaded and ready
   * @return true if model is ready for inference
   */
  virtual bool isReady() const = 0;

  /**
   * @brief Run depth estimation inference
   * @param rgb_image Input RGB image (CV_8UC3)
   * @return Depth image (CV_32FC1) with values in [0, 1] range (relative depth)
   */
  virtual cv::Mat infer(const cv::Mat & rgb_image) = 0;

  /**
   * @brief Get the expected input size for the model
   * @return cv::Size with expected width and height
   */
  virtual cv::Size getInputSize() const = 0;

  /**
   * @brief Get model name/identifier
   * @return Model name string
   */
  virtual std::string getName() const = 0;
};

}  // namespace nav2_depth_costmap

#endif  // NAV2_DEPTH_COSTMAP__DEPTH_MODEL__DEPTHMODEL_HPP_
