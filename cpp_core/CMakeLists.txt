cmake_minimum_required(VERSION 3.20)

project(ugv_cpp_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()


# ============================================================
# Core library
# ============================================================

add_library(ugv_core STATIC

    traversability/cost_map.cpp
    traversability/traversability_engine.cpp
    transport/shared_memory_transport.cpp
    transport/python_perception_reader.cpp
    transport/synchronized_perception_transport.cpp
    transport/metric_map_reader.cpp
    transport/synchronized_metric_map_transport.cpp
    transport/pose_frame.cpp
    transport/pose_frame_reader.cpp
    transport/synchronized_pose_transport.cpp

    planning/local_planner.cpp
    control/motion_controller.cpp
    control/safety_command.cpp
    ../mapping/metric_map_costmap_adapter.cpp
    runtime/ugv_runtime.cpp
)

target_include_directories(ugv_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)


# ============================================================
# Tests
# ============================================================

add_executable(test_robot_config
    tests/test_robot_config.cpp
)
target_link_libraries(test_robot_config PRIVATE ugv_core)


add_executable(test_robot_config_factory
    tests/test_robot_config_factory.cpp
)
target_link_libraries(test_robot_config_factory PRIVATE ugv_core)


add_executable(test_robot_config_adapter
    tests/test_robot_config_adapter.cpp
)
target_link_libraries(test_robot_config_adapter PRIVATE ugv_core)


add_executable(test_robot_geometry
    tests/test_robot_geometry.cpp
)
target_link_libraries(test_robot_geometry PRIVATE ugv_core)


add_executable(test_robot_footprint
    tests/test_robot_footprint.cpp
)
target_link_libraries(test_robot_footprint PRIVATE ugv_core)


add_executable(test_cost_map
    tests/test_cost_map.cpp
)
target_link_libraries(test_cost_map PRIVATE ugv_core)


add_executable(test_traversability
    tests/test_traversability.cpp
)
target_link_libraries(test_traversability PRIVATE ugv_core)


add_executable(test_grid_inflation
    tests/test_grid_inflation.cpp
)
target_link_libraries(test_grid_inflation PRIVATE ugv_core)


add_executable(test_grid_clearance
    tests/test_grid_clearance.cpp
)
target_link_libraries(test_grid_clearance PRIVATE ugv_core)


add_executable(test_grid_distance_transform_stress
    tests/test_grid_distance_transform_stress.cpp
)
target_link_libraries(
    test_grid_distance_transform_stress
    PRIVATE
        ugv_core
)


add_executable(test_local_planner
    tests/test_local_planner.cpp
)
target_link_libraries(test_local_planner PRIVATE ugv_core)

add_executable(
    test_perception_frame_header
    tests/test_perception_frame_header.cpp
)


target_link_libraries(
    test_perception_frame_header
    PRIVATE
    ugv_core
)
add_executable(test_shared_memory_transport
    tests/test_shared_memory_transport.cpp
)

target_link_libraries(test_shared_memory_transport
    PRIVATE
    ugv_core
)
add_executable(
    test_python_perception_reader
    tests/test_python_perception_reader.cpp
)
target_link_libraries(
    test_python_perception_reader
    PRIVATE
        ugv_core
)
add_executable(
    test_synchronized_perception_transport
    tests/test_synchronized_perception_transport.cpp
)
target_link_libraries(
    test_synchronized_perception_transport
    PRIVATE
        ugv_core
)
add_executable(
    test_live_perception_traversability
    tests/test_live_perception_traversability.cpp
)

target_link_libraries(
    test_live_perception_traversability
    PRIVATE
        ugv_core
)
add_executable(
    test_live_perception_planner
    tests/test_live_perception_planner.cpp
)

target_link_libraries(
    test_live_perception_planner
    PRIVATE
        ugv_core
)



add_executable(test_planner_cost_behavior
    tests/test_planner_cost_behavior.cpp
)
target_link_libraries(
    test_planner_cost_behavior
    PRIVATE
        ugv_core
)
add_executable(test_ugv_runtime tests/test_ugv_runtime.cpp)
target_link_libraries(test_ugv_runtime PRIVATE ugv_core)

add_executable(test_synchronized_metric_map_transport
    tests/test_synchronized_metric_map_transport.cpp
)

target_link_libraries(test_synchronized_metric_map_transport
    PRIVATE
    ugv_core
)
