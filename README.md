# navigation2.ai

<p align="center">
  <img src="https://img.shields.io/badge/ROS2-Humble-blue" alt="ROS2 Humble"/>
  <img src="https://img.shields.io/badge/License-Apache%202.0-green" alt="License"/>
  <img src="https://img.shields.io/badge/ONNX%20Runtime-1.16.3-orange" alt="ONNX Runtime"/>
</p>

AI-powered perception modules for the [Nav2](https://github.com/ros-planning/navigation2) navigation stack. Transform RGB camera images into 3D obstacle data using state-of-the-art deep learning models.

## 🎯 Overview

This repository bridges the gap between modern AI vision models and robot navigation. It enables robots equipped with standard RGB cameras to perceive depth and obstacles without expensive depth sensors like LiDAR or stereo cameras.

**Key Features:**
- 🧠 **Monocular Depth Estimation** – Generate depth maps from single RGB images
- 🔌 **Nav2 Integration** – Direct PointCloud2 output for VoxelLayer costmaps
- ⚡ **ONNX Runtime** – Optimized inference with CPU/GPU support
- 🐳 **Docker Ready** – Fully containerized development environment
- 🔄 **Model Agnostic** – Easily swap depth estimation models

## 📦 Packages

| Package | Description |
|---------|-------------|
| **nav2_depth_costmap** | Depth estimation pipeline converting RGB images to PointCloud2 for obstacle avoidance |

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           RGB Camera Input                                   │
│                                  │                                           │
│                                  ▼                                           │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                      DepthEstimationNode                             │    │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────┐  │    │
│  │  │ImagePreprocessor│─▶│   OnnxModel     │─▶│PointCloudGenerator  │  │    │
│  │  │  • Resize       │  │  (Depth         │  │  • Back-projection  │  │    │
│  │  │  • Normalize    │  │   Anything V2)  │  │  • Depth filtering  │  │    │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────────┘  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                  │                                           │
│                    ┌─────────────┴─────────────┐                            │
│                    ▼                           ▼                            │
│            ┌──────────────┐           ┌──────────────┐                      │
│            │ Depth Image  │           │ PointCloud2  │                      │
│            │   (32FC1)    │           │   (XYZ)      │                      │
│            └──────────────┘           └──────┬───────┘                      │
│                                              │                              │
│                                              ▼                              │
│                                     ┌──────────────┐                        │
│                                     │ Nav2 Voxel   │                        │
│                                     │    Layer     │                        │
│                                     └──────────────┘                        │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 🚀 Quick Start

### Prerequisites

- ROS 2 Humble
- NVIDIA GPU (optional, for CUDA acceleration)
- Docker & Docker Compose (recommended)

### Using Docker (Recommended)

```bash
# Clone the repository
git clone https://github.com/xFranv8/navigation2.ai.git
cd navigation2.ai

# Build and start the container
docker compose -f docker/docker-compose.yml build
docker compose -f docker/docker-compose.yml run dev

# Inside the container
cd /ros2_ws/navigation2.ai
colcon build --symlink-install
source install/setup.bash
```

### Native Installation

```bash
# Install dependencies
sudo apt install ros-humble-cv-bridge ros-humble-image-transport \
    ros-humble-sensor-msgs ros-humble-tf2-ros

# Install ONNX Runtime (v1.16.3)
cd /tmp
wget https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-linux-x64-1.16.3.tgz
tar -xzf onnxruntime-linux-x64-1.16.3.tgz
sudo cp -P onnxruntime-linux-x64-1.16.3/lib/* /usr/local/lib/
sudo cp -r onnxruntime-linux-x64-1.16.3/include/* /usr/local/include/
sudo ldconfig

# Build the package
cd ~/ros2_ws
colcon build --packages-select nav2_depth_costmap
source install/setup.bash
```

## 🧪 Model Setup

This package uses [Depth Anything V2](https://github.com/DepthAnything/Depth-Anything-V2) for monocular depth estimation.

### Download and Convert Model

```bash
# Clone Depth Anything V2 (required for model architecture)
git clone https://github.com/DepthAnything/Depth-Anything-V2.git

# Download pretrained weights (ViT-S for speed, ViT-L for quality)
# See: https://github.com/DepthAnything/Depth-Anything-V2#pre-trained-models

# Convert PyTorch model to ONNX
python3 nav2_depth_costmap/scripts/convert_pth_to_onnx.py \
    --input ./Depth-Anything-V2/checkpoints/depth_anything_v2_vits.pth \
    --output ./model.onnx \
    --encoder vits \
    --depth-anything-path ./Depth-Anything-V2
```

### Model Options

| Encoder | Parameters | Speed   | Quality | Use Case |
|---------|------------|---------|---------|----------|
| `vits`  | 25M        | ~30 FPS | Good    | Real-time robotics |
| `vitb`  | 98M        | ~15 FPS | Better  | Balanced |
| `vitl`  | 335M       | ~5 FPS  | Best    | High accuracy |

## 🎮 Usage

### Basic Launch

```bash
ros2 launch nav2_depth_costmap depth_estimation.launch.py \
    model_path:=$(pwd)/model.onnx
```

### With USB Camera

```bash
# Terminal 1: Start camera
ros2 run usb_cam usb_cam_node_exe --ros-args \
    -p video_device:=/dev/video0 \
    -r image_raw:=/image_raw

# Terminal 2: Start depth estimation
ros2 launch nav2_depth_costmap depth_estimation.launch.py \
    model_path:=$(pwd)/model.onnx \
    image_topic:=/image_raw

# Terminal 3: Visualize
rviz2
```

### Launch Arguments

| Argument | Default | Description |
|----------|---------|-------------|
| `model_path` | (required) | Path to ONNX model file |
| `image_topic` | `/camera/image_raw` | Input RGB image topic |
| `camera_info_topic` | `/camera/camera_info` | Camera intrinsics topic |
| `pointcloud_topic` | `/depth_estimation/points` | Output PointCloud2 topic |
| `use_sim_time` | `false` | Use simulation time |

## ⚙️ Configuration

Edit `config/depth_estimation_params.yaml`:

```yaml
depth_estimation_node:
  ros__parameters:
    # Model settings
    model_input_width: 518
    model_input_height: 518
    use_gpu: true

    # Depth range (meters)
    min_depth: 0.1
    max_depth: 10.0
    
    # Topics
    image_topic: "/camera/image_raw"
    pointcloud_topic: "/depth_estimation/points"
    depth_image_topic: "/depth_estimation/depth"
```

## 🗺️ Nav2 Integration

Add the depth pointcloud to your costmap configuration:

```yaml
local_costmap:
  local_costmap:
    ros__parameters:
      plugins: ["voxel_layer", "inflation_layer"]
      voxel_layer:
        plugin: "nav2_costmap_2d::VoxelLayer"
        enabled: true
        observation_sources: depth_camera
        depth_camera:
          topic: /depth_estimation/points
          data_type: PointCloud2
          marking: true
          clearing: true
          min_obstacle_height: 0.1
          max_obstacle_height: 2.0
          obstacle_max_range: 10.0
```

## 📡 Topics

### Subscribed
| Topic | Type | Description |
|-------|------|-------------|
| `/camera/image_raw` | `sensor_msgs/Image` | RGB camera input |
| `/camera/camera_info` | `sensor_msgs/CameraInfo` | Camera intrinsics |

### Published
| Topic | Type | Description |
|-------|------|-------------|
| `/depth_estimation/points` | `sensor_msgs/PointCloud2` | 3D obstacle pointcloud |
| `/depth_estimation/depth` | `sensor_msgs/Image` | Depth image (32FC1) |

## 🛠️ Development

### Project Structure

```
navigation2.ai/
├── nav2_depth_costmap/          # Main ROS 2 package
│   ├── include/                 # C++ headers
│   ├── src/                     # C++ source files
│   ├── config/                  # Parameter files
│   ├── launch/                  # Launch files
│   └── scripts/                 # Python utilities
├── Depth-Anything-V2/           # Depth model (submodule)
├── docker/                      # Container configuration
├── model.onnx                   # Converted model
└── README.md
```

### Extending with New Models

Implement the `DepthModel` interface:

```cpp
class DepthModel {
public:
  virtual bool load(const std::string& model_path) = 0;
  virtual bool isReady() const = 0;
  virtual cv::Mat infer(const cv::Mat& rgb_image) = 0;
  virtual cv::Size getInputSize() const = 0;
  virtual std::string getName() const = 0;
};
```

## 📄 License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [Depth Anything V2](https://github.com/DepthAnything/Depth-Anything-V2) - State-of-the-art monocular depth estimation
- [Nav2](https://github.com/ros-planning/navigation2) - ROS 2 Navigation Stack
- [ONNX Runtime](https://onnxruntime.ai/) - High-performance ML inference

## 📬 Contact

- **Author**: xfranv8
- **Repository**: [github.com/xFranv8/navigation2.ai](https://github.com/xFranv8/navigation2.ai)
- **Issues**: [Bug Reports](https://github.com/xFranv8/navigation2.ai/issues)
