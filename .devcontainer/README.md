# Dev Container for Nav2 Depth Costmap

This development container provides a complete ROS 2 Jazzy environment with all dependencies pre-installed for developing and testing the nav2_depth_costmap package.

## What's Included

- **ROS 2 Jazzy Desktop Full** - Complete ROS 2 installation with RViz, rqt, and visualization tools
- **Build Tools** - colcon, CMake, C++ compiler
- **ROS Dependencies** - cv_bridge, image_transport, tf2, sensor_msgs
- **ONNX Runtime** - v1.16.3 with CPU support (GPU if available)
- **Python ML Tools** - PyTorch, ONNX, OpenCV for model conversion
- **Development Tools** - gdb, git, vim, VSCode extensions
- **Camera Support** - usb_cam for testing with real cameras

## Prerequisites

1. **Docker** - Install [Docker Desktop](https://www.docker.com/products/docker-desktop/)
2. **VSCode** - Install [Visual Studio Code](https://code.visualstudio.com/)
3. **Dev Containers Extension** - Install from VSCode marketplace

### For RViz (GUI) Support

On **Linux**:
```bash
# Allow X11 forwarding
xhost +local:docker
```

On **macOS**:
- Install [XQuartz](https://www.xquartz.org/)
- Enable "Allow connections from network clients" in XQuartz preferences
- Run: `xhost +localhost`

On **Windows**:
- Install [VcXsrv](https://sourceforge.net/projects/vcxsrv/)
- Launch XLaunch with "Disable access control" checked

### For GPU Support (Optional)

Install [NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/install-guide.html) for CUDA acceleration.

## Quick Start

1. **Open in VSCode**:
   ```bash
   cd /path/to/navigation2.ai
   code .
   ```

2. **Reopen in Container**:
   - Press `F1` or `Ctrl+Shift+P`
   - Select: `Dev Containers: Reopen in Container`
   - Wait for container to build (first time takes ~5-10 minutes)

3. **Verify Setup**:
   ```bash
   # Check ROS installation
   ros2 --version

   # Check workspace build
   colcon list

   # Check ONNX Runtime
   python3 -c "import onnxruntime; print(f'ONNX Runtime: {onnxruntime.__version__}')"
   ```

## Usage

### Build the Package

```bash
# Option 1: Use alias
build

# Option 2: Manual
cd /workspace
colcon build --symlink-install --packages-select nav2_depth_costmap
source install/setup.bash
```

### Convert Model to ONNX

```bash
# Clone Depth Anything V2 (one-time setup)
cd ~
git clone https://github.com/DepthAnything/Depth-Anything-V2.git

# Convert your .pth model
cd /workspace/nav2_depth_costmap
python3 scripts/convert_pth_to_onnx.py \
    --input ~/depth_anything_v2_vits.pth \
    --output ~/depth_anything_v2_vits.onnx \
    --encoder vits \
    --depth-anything-path ~/Depth-Anything-V2
```

### Test with Camera

#### Option 1: USB Camera
```bash
# Terminal 1: Start camera
ros2 run usb_cam usb_cam_node_exe

# Terminal 2: Run depth estimation
ros2 launch nav2_depth_costmap depth_estimation.launch.py \
    model_path:=~/depth_anything_v2_vits.onnx

# Terminal 3: Visualize
rviz2
```

#### Option 2: Image File Testing
```bash
# Publish a static image
ros2 run image_publisher image_publisher_node /path/to/image.jpg
```

### Run Tests

```bash
# Option 1: Use alias
test

# Option 2: Manual
colcon test --packages-select nav2_depth_costmap
colcon test-result --verbose
```

## Useful Aliases

The container comes with these pre-configured aliases:

- `build` - Build the workspace
- `test` - Run tests
- `source_ws` - Source the workspace

## Troubleshooting

### RViz doesn't open
```bash
# Check X11 forwarding
echo $DISPLAY  # Should show something like :0 or :1

# On host machine, allow Docker to connect
xhost +local:docker
```

### GPU not detected
```bash
# Check NVIDIA runtime
nvidia-smi  # Should show GPU info

# If not working, remove --gpus flag from devcontainer.json runArgs
```

### Build fails
```bash
# Clean and rebuild
rm -rf /workspace/build /workspace/install
colcon build --packages-select nav2_depth_costmap
```

### Python packages missing
```bash
# Install additional packages
pip3 install <package-name>
```

## Development Workflow

1. **Edit code** in VSCode (outside container or inside)
2. **Build** with `build` command
3. **Test** with camera or rosbag
4. **Debug** with VSCode debugger (C++) or pdb (Python)
5. **Commit** changes (Git works inside and outside container)

## Advanced Configuration

### Add More ROS Packages

Edit `.devcontainer/Dockerfile`:
```dockerfile
RUN apt-get update && apt-get install -y \
    ros-jazzy-your-package \
    && rm -rf /var/lib/apt/lists/*
```

### Mount Additional Volumes

Edit `.devcontainer/devcontainer.json`:
```json
"mounts": [
    "source=/path/on/host,target=/path/in/container,type=bind"
]
```

### Change ROS Domain ID

Edit `.devcontainer/devcontainer.json`:
```json
"containerEnv": {
    "ROS_DOMAIN_ID": "your_id"
}
```

## Performance Tips

- Use `--symlink-install` for faster rebuilds (already default)
- Mount your workspace on SSD for better I/O
- Allocate more resources to Docker Desktop if builds are slow
- Use `colcon build --packages-select` to build only what you need

## Notes

- The workspace is at `/workspace` (mounted from host)
- First build happens automatically in `postCreateCommand`
- All changes to code are persistent (volume mounted)
- Container state is ephemeral (rebuild to reset)
