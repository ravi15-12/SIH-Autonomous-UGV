#include "motion_controller.hpp"

#include <algorithm>
#include <cmath>

namespace ugv {

bool MotionControllerConfig::isValid() const noexcept {
    return std::isfinite(max_linear_velocity_mps) &&
           max_linear_velocity_mps > 0.0 &&
           std::isfinite(max_angular_velocity_rps) &&
           max_angular_velocity_rps > 0.0 &&
           std::isfinite(lookahead_distance_m) &&
           lookahead_distance_m > 0.0 &&
           std::isfinite(position_tolerance_m) &&
           position_tolerance_m >= 0.0 &&
           std::isfinite(heading_gain) &&
           heading_gain >= 0.0;
}

MotionController::MotionController(
    const MotionControllerConfig& config
)
    : config_(config) {}

PlannerCommand MotionController::compute(
    const Path& path,
    const PlannerPose& robot_pose
) const noexcept {

    PlannerCommand command{};

    if (!config_.isValid()) {
        return command;
    }

    if (!path.valid || path.points.empty()) {
        return command;
    }

    if (!std::isfinite(robot_pose.x_m) ||
        !std::isfinite(robot_pose.y_m) ||
        !std::isfinite(robot_pose.yaw_rad)) {
        return command;
    }

    const double lookahead_sq =
        config_.lookahead_distance_m *
        config_.lookahead_distance_m;

    const PathPoint* target = nullptr;

    for (const auto& point : path.points) {
        const double dx = point.x_m - robot_pose.x_m;
        const double dy = point.y_m - robot_pose.y_m;

        if ((dx * dx + dy * dy) >= lookahead_sq) {
            target = &point;
            break;
        }
    }

    if (target == nullptr) {
        target = &path.points.back();
    }

    const double dx =
        target->x_m - robot_pose.x_m;

    const double dy =
        target->y_m - robot_pose.y_m;

    const double distance =
        std::hypot(dx, dy);

    if (!std::isfinite(distance)) {
        return command;
    }

    if (distance <= config_.position_tolerance_m) {
        return command;
    }

    const double target_heading =
        std::atan2(dy, dx);

    double heading_error =
        target_heading - robot_pose.yaw_rad;

    while (heading_error > M_PI) {
        heading_error -= 2.0 * M_PI;
    }

    while (heading_error < -M_PI) {
        heading_error += 2.0 * M_PI;
    }

    double angular_velocity =
        config_.heading_gain * heading_error;

    angular_velocity = std::clamp(
        angular_velocity,
        -config_.max_angular_velocity_rps,
        config_.max_angular_velocity_rps
    );

    double linear_velocity =
        config_.max_linear_velocity_mps;

    const double heading_factor =
        std::max(
            0.0,
            std::cos(heading_error)
        );

    linear_velocity *= heading_factor;

    linear_velocity = std::clamp(
        linear_velocity,
        0.0,
        config_.max_linear_velocity_mps
    );

    command.linear_velocity_mps =
        linear_velocity;

    command.angular_velocity_rps =
        angular_velocity;

    command.valid = true;

    return command;
}

}  // namespace ugv
