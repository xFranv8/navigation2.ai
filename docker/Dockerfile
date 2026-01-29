FROM osrf/ros:humble-desktop-full

ARG UID
ARG USER

USER root

ENV DEBIAN_FRONTEND noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    curl \
    git \
    wget \
    python3-pip \
    # Project specific dependencies
    # libonnxruntime-dev is not available in Ubuntu 22.04, installed manually below
    ros-humble-cv-bridge \
    ros-humble-image-transport \
    ros-humble-sensor-msgs \
    ros-humble-tf2-ros \
    ros-humble-tf2-geometry-msgs \
    ros-humble-ament-lint-auto \
    ros-humble-ament-lint-common \
    ros-humble-ament-cmake-gtest \
    && rm -rf /var/lib/apt/lists/*

# Install ONNX Runtime (Manual installation for Ubuntu 22.04)
RUN cd /tmp && \
    wget -q https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-linux-x64-1.16.3.tgz && \
    tar -xzf onnxruntime-linux-x64-1.16.3.tgz && \
    cp -P onnxruntime-linux-x64-1.16.3/lib/* /usr/local/lib/ && \
    cp -r onnxruntime-linux-x64-1.16.3/include/* /usr/local/include/ && \
    ldconfig && \
    rm -rf onnxruntime-linux-x64-1.16.3*

# Install Python dependencies for model conversion
# RUN pip3 install torch torchvision onnx

RUN if getent passwd 1000 > /dev/null; then \
        USERNAME_TO_DELETE=$(getent passwd 1000 | cut -d: -f1); \
        userdel -r "$USERNAME_TO_DELETE"; \
    else \
        echo "User with UID 1000 not found"; \
    fi

RUN adduser --uid $UID --gecos "Nav2 Developer" --disabled-password $USER
RUN adduser $USER sudo
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers
RUN echo "export QT_X11_NO_MITSHM=1" >> /home/$USER/.bashrc
RUN echo "export PATH=${PATH}:/home/${USER}/.local/bin" >> /home/$USER/.bashrc

USER ${USER}

WORKDIR /ros2_ws/navigation2.ai

RUN export PATH=${PATH}:/home/${USER}/.local/bin

# Source ROS setup in .bashrc
RUN echo "source /opt/ros/humble/setup.bash" >> /home/$USER/.bashrc

ENTRYPOINT ["/bin/bash"]


