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

#include <stdexcept>
#include <algorithm>
#include <iostream>

#include "nav2_depth_costmap/depth_model/DepthAnythingV2.hpp"

namespace nav2_depth_costmap
{

DepthAnythingV2::DepthAnythingV2(int input_width, int input_height, bool use_gpu)
: input_width_(input_width),
  input_height_(input_height),
  use_gpu_(use_gpu),
  is_ready_(false),
  model_name_("DepthAnythingV2") {
}

bool DepthAnythingV2::load(const std::string & model_path) {
  try {
    // Create ONNX Runtime environment
    env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "DepthEstimation");

    // Configure session options
    session_options_ = std::make_unique<Ort::SessionOptions>();
    session_options_->SetIntraOpNumThreads(1);
    session_options_->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

    // Try to use CUDA if requested and available
    if (use_gpu_) {
      try {
        OrtCUDAProviderOptions cuda_options;
        cuda_options.device_id = 0;
        session_options_->AppendExecutionProvider_CUDA(cuda_options);
        std::cout << "[OnnxDepthModel] Using CUDA execution provider" << std::endl;
      } catch (const Ort::Exception & e) {
        std::cout << "[OnnxDepthModel] CUDA not available, falling back to CPU: "
                  << e.what() << std::endl;
      }
    }

    // Create session
    session_ = std::make_unique<Ort::Session>(*env_, model_path.c_str(), *session_options_);

    // Get input info
    Ort::AllocatorWithDefaultOptions allocator;
    auto input_name_ptr = session_->GetInputNameAllocated(0, allocator);
    input_name_ = input_name_ptr.get();

    auto input_info = session_->GetInputTypeInfo(0);
    auto input_tensor_info = input_info.GetTensorTypeAndShapeInfo();
    input_shape_ = input_tensor_info.GetShape();

    // Handle dynamic dimensions (-1) in input shape
    if (input_shape_.size() == 4) {
      if (input_shape_[2] <= 0) input_shape_[2] = input_height_;
      if (input_shape_[3] <= 0) input_shape_[3] = input_width_;
    }

    // Get output info
    auto output_name_ptr = session_->GetOutputNameAllocated(0, allocator);
    output_name_ = output_name_ptr.get();

    auto output_info = session_->GetOutputTypeInfo(0);
    auto output_tensor_info = output_info.GetTensorTypeAndShapeInfo();
    output_shape_ = output_tensor_info.GetShape();

    // Extract model name from path
    size_t last_slash = model_path.find_last_of("/\\");
    size_t last_dot = model_path.find_last_of(".");
    if (last_slash != std::string::npos && last_dot != std::string::npos) {
      model_name_ = model_path.substr(last_slash + 1, last_dot - last_slash - 1);
    }

    is_ready_ = true;
    std::cout << "[OnnxDepthModel] Model loaded: " << model_name_ << std::endl;
    std::cout << "[OnnxDepthModel] Input: " << input_name_ << " shape: [";
    for (size_t i = 0; i < input_shape_.size(); ++i) {
      std::cout << input_shape_[i];
      if (i < input_shape_.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    return true;
  } catch (const Ort::Exception & e) {
    std::cerr << "[OnnxDepthModel] Failed to load model: " << e.what() << std::endl;
    is_ready_ = false;
    return false;
  }
}

bool DepthAnythingV2::isReady() const {
  return is_ready_;
}

cv::Mat DepthAnythingV2::infer(const cv::Mat & rgb_image) {
  if (!is_ready_) {
    throw std::runtime_error("Model not loaded");
  }

  cv::Size original_size = rgb_image.size();

  // Preprocess
  std::vector<float> input_tensor_values = preprocess(rgb_image);

  // Create input tensor
  std::vector<int64_t> input_shape = {1, 3, input_height_, input_width_};
  auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
  Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
    memory_info,
    input_tensor_values.data(),
    input_tensor_values.size(),
    input_shape.data(),
    input_shape.size());

  // Run inference
  const char * input_names[] = {input_name_.c_str()};
  const char * output_names[] = {output_name_.c_str()};

  auto output_tensors = session_->Run(
    Ort::RunOptions{nullptr},
    input_names, &input_tensor, 1,
    output_names, 1);

  // Get output
  float * output_data = output_tensors[0].GetTensorMutableData<float>();
  auto output_info = output_tensors[0].GetTensorTypeAndShapeInfo();
  auto output_shape = output_info.GetShape();

  // Determine output dimensions
  int output_height, output_width;
  if (output_shape.size() == 4) {
    output_height = static_cast<int>(output_shape[2]);
    output_width = static_cast<int>(output_shape[3]);
  } else if (output_shape.size() == 3) {
    output_height = static_cast<int>(output_shape[1]);
    output_width = static_cast<int>(output_shape[2]);
  } else if (output_shape.size() == 2) {
    output_height = static_cast<int>(output_shape[0]);
    output_width = static_cast<int>(output_shape[1]);
  } else {
    throw std::runtime_error("Unexpected output tensor shape");
  }

  return postprocess(output_data, output_height, output_width, original_size);
}

cv::Size DepthAnythingV2::getInputSize() const {
  return cv::Size(input_width_, input_height_);
}

std::string DepthAnythingV2::getName() const {
  return model_name_;
}

std::vector<float> DepthAnythingV2::preprocess(const cv::Mat & image) {
  cv::Mat rgb;
  if (image.channels() == 3) {
    cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);
  } else {
    rgb = image.clone();
  }

  // Resize to model input size
  cv::Mat resized;
  cv::resize(rgb, resized, cv::Size(input_width_, input_height_), 0, 0, cv::INTER_LINEAR);

  // Convert to float and normalize
  cv::Mat float_image;
  resized.convertTo(float_image, CV_32FC3, 1.0 / 255.0);

  // Prepare tensor data in NCHW format with ImageNet normalization
  std::vector<float> tensor_data(3 * input_height_ * input_width_);

  for (int y = 0; y < input_height_; ++y) {
    for (int x = 0; x < input_width_; ++x) {
      cv::Vec3f pixel = float_image.at<cv::Vec3f>(y, x);
      int idx = y * input_width_ + x;

      // RGB order, normalized with ImageNet mean/std
      tensor_data[0 * input_height_ * input_width_ + idx] = (pixel[0] - MEAN_R) / STD_R;
      tensor_data[1 * input_height_ * input_width_ + idx] = (pixel[1] - MEAN_G) / STD_G;
      tensor_data[2 * input_height_ * input_width_ + idx] = (pixel[2] - MEAN_B) / STD_B;
    }
  }

  return tensor_data;
}

cv::Mat DepthAnythingV2::postprocess(
  const float * output_data,
  int output_height, int output_width,
  const cv::Size & original_size) {
  // Create depth image from output
  cv::Mat depth(output_height, output_width, CV_32FC1);
  std::memcpy(depth.data, output_data, output_height * output_width * sizeof(float));

  // Normalize to [0, 1] range
  double min_val, max_val;
  cv::minMaxLoc(depth, &min_val, &max_val);
  if (max_val > min_val) {
    depth = (depth - min_val) / (max_val - min_val);
  }

  // Invert: Depth Anything V2 outputs disparity (high = close)
  // We need depth (high = far) for correct 3D projection
  // depth = 1.0f - depth;

  // Resize back to original size
  cv::Mat depth_resized;
  cv::resize(depth, depth_resized, original_size, 0, 0, cv::INTER_LINEAR);

  return depth_resized;
}

}  // namespace nav2_depth_costmap
