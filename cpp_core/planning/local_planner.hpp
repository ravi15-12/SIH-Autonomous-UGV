#pragma once

#include <cstdint>
#include "path.hpp"

#include "../traversability/cost_map.hpp"
#include "../common/robot_footprint.hpp"
#include "../common/robot_config.hpp"

namespace ugv {

struct PlannerConfig {
    // CostMap interpretation.
    std::uint8_t traversable_threshold = 30;
    std::uint8_t blocked_threshold = 100;

    // Local planning boundary.
    double max_goal_distance_m = 5.0;

    // Safety clearance around the robot.
    // Provisional until the real UGV footprint is measured.
    double minimum_clearance_m = 0.20;

    // Candidate-path cost weights.
    // These are provisional and must be tuned/validated
    // against real outdoor navigation data.
    double path_cost_weight = 1.0;
    double path_length_weight = 1.0;
    double clearance_weight = 2.0;

    // Search limits.
    // Prevents unbounded computation on invalid or pathological maps.
    std::size_t maximum_path_points = 500;
    std::size_t maximum_search_nodes = 20'000;

    RobotConfig robot_config{};
};

struct PlannerPose {
    // Robot position in the local planning frame, metres.
    double x_m = 0.0;
    double y_m = 0.0;

    // Robot heading, radians.
    double yaw_rad = 0.0;
};

struct PlannerGoal {
    // Goal position in the local planning frame, metres.
    double x_m = 0.0;
    double y_m = 0.0;
};

struct PlannerCommand {
    // Desired linear velocity, m/s.
    double linear_velocity_mps = 0.0;

    // Desired angular velocity, rad/s.
    double angular_velocity_rps = 0.0;

    // False means the planner has no safe command.
    bool valid = false;
};

class LocalPlanner {
public:
    explicit LocalPlanner(const PlannerConfig& config);
    Path planPath(
    const CostMap& cost_map,
    const PlannerPose& robot_pose,
    const PlannerGoal& goal
    ) const noexcept;

    PlannerCommand compute(
        const CostMap& cost_map,
        const PlannerPose& robot_pose,
        const PlannerGoal& goal
    ) const noexcept;

private:
    PlannerConfig config_;

    bool isInputValid(
        const CostMap& cost_map,
        const PlannerPose& robot_pose,
        const PlannerGoal& goal
    ) const noexcept;
};

}  // namespace ugv
