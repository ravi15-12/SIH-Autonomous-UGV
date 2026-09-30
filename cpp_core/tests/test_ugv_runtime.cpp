#include "../runtime/ugv_runtime.hpp"

#include <iostream>

int main() {
    using namespace ugv;

    UgvRuntimeConfig config;

    config.map_geometry.resolution_m = 0.05;
    config.map_geometry.forward_min_m = 0.0;
    config.map_geometry.left_min_m = -4.0;

    config.planner_config.robot_config.geometry.length_m = 0.20;
    config.planner_config.robot_config.geometry.width_m = 0.20;
    config.planner_config.robot_config.geometry.wheel_base_m = 0.15;
    config.planner_config.robot_config.geometry.track_width_m = 0.15;
    config.planner_config.robot_config.geometry.front_overhang_m = 0.025;
    config.planner_config.robot_config.geometry.rear_overhang_m = 0.025;

    config.planner_config.minimum_clearance_m = 0.0;
    config.planner_config.max_goal_distance_m = 5.0;

    config.controller_config.max_linear_velocity_mps = 1.0;
    config.controller_config.max_angular_velocity_rps = 1.0;
    config.controller_config.lookahead_distance_m = 0.20;
    config.controller_config.position_tolerance_m = 0.05;
    config.controller_config.heading_gain = 1.5;

    config.robot_pose = PlannerPose{0.10, -0.10, 0.0};
    config.goal = PlannerGoal{1.00, -0.10};

    UgvRuntime runtime(config);

    if (!runtime.open()) {
        std::cerr << "ERROR: runtime failed to open\n";
        return 1;
    }

    constexpr int kFrameCount = 10;

    for (int i = 0; i < kFrameCount; ++i) {
        UgvRuntimeOutput output;

        if (!runtime.step(output)) {
            std::cerr << "ERROR: runtime step failed at frame "
                      << (i + 1) << "\n";
            runtime.close();
            return 1;
        }

        std::cout
            << "FRAME: " << (i + 1)
            << " | MAP: "
            << (output.map_valid ? "VALID" : "INVALID")
            << " | FOOTPRINT: "
            << (output.robot_footprint_observed ? "OBSERVED" : "UNKNOWN")
            << " | PATH: "
            << (output.path.valid ? "VALID" : "NO")
            << " | SAFETY: "
            << (output.safety.state == SafetyState::SafeToMove
                    ? "SAFE_TO_MOVE"
                    : "STOP")
            << " | LINEAR: "
            << output.safety.command.linear_velocity_mps
            << " | ANGULAR: "
            << output.safety.command.angular_velocity_rps
            << "\n";
    }

    runtime.close();

    std::cout << "UGV CONTINUOUS RUNTIME: PASS\n";

    return 0;
}