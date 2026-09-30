#pragma once

#include "../planning/local_planner.hpp"

namespace ugv {

struct MotionControllerConfig {
    double max_linear_velocity_mps = 1.0;
    double max_angular_velocity_rps = 1.0;

    double lookahead_distance_m = 0.50;
    double position_tolerance_m = 0.10;
    double heading_gain = 1.5;

    bool isValid() const noexcept;
};

class MotionController {
public:
    explicit MotionController(
        const MotionControllerConfig& config
    );

    PlannerCommand compute(
        const Path& path,
        const PlannerPose& robot_pose
    ) const noexcept;

private:
    MotionControllerConfig config_;
};

}  // namespace ugv
