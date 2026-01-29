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

#ifndef NAV2_DEPTH_COSTMAP__DEPTH_MODEL__ONNXDEPTHMODEL_HPP_
#define NAV2_DEPTH_COSTMAP__DEPTH_MODEL__ONNXDEPTHMODEL_HPP_

#include <onnxruntime_cxx_api.h>

#include <memory>
#include <string>
#include <vector>

#include "opencv2/opencv.hpp"

#include "nav2_depth_costmap/depth_model/DepthModel.hpp"

namespace nav2_depth_costmap
{

/**
 * @class OnnxDepthModel
 * @brief ONNX Runtime implementation of the DepthModel interface.
 *
 * This class loads and runs depth estimation models in ONNX format.
 * It supports any model that follows the standard input/output format:
 * - Input: [1, 3, H, W] float32 tensor (normalized RGB)
 * - Output: [1, 1, H, W] or [1, H, W] float32 tensor (relative depth)
 */
class OnnxDepthModel : public DepthModel {
public:
  /**
   * @brief Constructor
   * @param input_width Expected input width for the model
   * @param input_height Expected input height for the model
   * @param use_gpu Whether to use GPU acceleration (CUDA)
   */
  explicit OnnxDepthModel(int input_width = 518, int input_height = 518, bool use_gpu = true);

  ~OnnxDepthModel() override = default;

  bool load(const std::string & model_path) override;
  bool isReady() const override;
  cv::Mat infer(const cv::Mat & rgb_image) override;
  cv::Size getInputSize() const override;
  std::string getName() const override;

private:
  /**
   * @brief Preprocess image for model input
   * @param image Input BGR image
   * @return Preprocessed tensor data as vector
   */
  std::vector<float> preprocess(const cv::Mat & image);

  /**
   * @brief Postprocess model output to depth image
   * @param output_data Raw output tensor data
   * @param output_height Output tensor height
   * @param output_width Output tensor width
   * @param original_size Original image size for resizing
   * @return Depth image (CV_32FC1) normalized to [0, 1]
   */
  cv::Mat postprocess(
    const float * output_data,
    int output_height, int output_width,
    const cv::Size & original_size);

  std::unique_ptr<Ort::Env> env_;
  std::unique_ptr<Ort::Session> session_;
  std::unique_ptr<Ort::SessionOptions> session_options_;

  std::string input_name_;
  std::string output_name_;
  std::vector<int64_t> input_shape_;
  std::vector<int64_t> output_shape_;

  int input_width_;
  int input_height_;
  bool use_gpu_;
  bool is_ready_;
  std::string model_name_;

  // Normalization constants (ImageNet standard)
  static constexpr float MEAN_R = 0.485f;
  static constexpr float MEAN_G = 0.456f;
  static constexpr float MEAN_B = 0.406f;
  static constexpr float STD_R = 0.229f;
  static constexpr float STD_G = 0.224f;
  static constexpr float STD_B = 0.225f;
};

}  // namespace nav2_depth_costmap

#endif  // NAV2_DEPTH_COSTMAP__DEPTH_MODEL__ONNXDEPTHMODEL_HPP_
