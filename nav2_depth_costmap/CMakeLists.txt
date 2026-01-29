cmake_minimum_required(VERSION 3.8)
project(nav2_depth_costmap)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)

# Find ONNX Runtime
find_library(ONNXRUNTIME_LIB onnxruntime)
find_path(ONNXRUNTIME_INCLUDE onnxruntime_cxx_api.h
  HINTS /usr/include/onnxruntime /usr/local/include/onnxruntime)

if(NOT ONNXRUNTIME_LIB)
  message(FATAL_ERROR "ONNX Runtime library not found. Please install libonnxruntime-dev")
endif()

set(dependencies
  rclcpp
  rclcpp_components
  sensor_msgs
  std_msgs
  cv_bridge
)

include_directories(
  include
  ${OpenCV_INCLUDE_DIRS}
  ${ONNXRUNTIME_INCLUDE}
)

# Library with all core components
add_library(${PROJECT_NAME} SHARED
  src/nav2_depth_costmap/depth_model/OnnxDepthModel.cpp
  src/nav2_depth_costmap/ImagePreprocessor.cpp
  src/nav2_depth_costmap/PointCloudGenerator.cpp
  src/nav2_depth_costmap/DepthEstimationNode.cpp
)
target_link_libraries(${PROJECT_NAME}
  ${OpenCV_LIBRARIES}
  ${ONNXRUNTIME_LIB}
)
ament_target_dependencies(${PROJECT_NAME} ${dependencies})

# Register node as component
rclcpp_components_register_nodes(${PROJECT_NAME}
  "nav2_depth_costmap::DepthEstimationNode"
)

# Standalone executable
add_executable(depth_estimation_node src/depth_estimation_main.cpp)
target_link_libraries(depth_estimation_node ${PROJECT_NAME})
ament_target_dependencies(depth_estimation_node ${dependencies})

# Install targets
install(TARGETS
  ${PROJECT_NAME}
  depth_estimation_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

# Install headers
install(DIRECTORY include/ DESTINATION include)

# Install launch files
install(DIRECTORY launch/ DESTINATION share/${PROJECT_NAME}/launch)

# Install config files
install(DIRECTORY config/ DESTINATION share/${PROJECT_NAME}/config)

# Install scripts
install(PROGRAMS
  scripts/convert_pth_to_onnx.py
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)

  # Skip uncrustify - using intentional brace-on-same-line style
  set(ament_cmake_uncrustify_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)

  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)
  # add_subdirectory(tests)
endif()

ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})
ament_export_dependencies(${dependencies})

ament_package()
