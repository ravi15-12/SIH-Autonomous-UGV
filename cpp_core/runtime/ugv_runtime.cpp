#include "ugv_runtime.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace ugv {

namespace {

bool isRobotFootprintObserved(
    const MetricMapFrame& frame,
    const MetricMapGeometry& geometry,
    const RobotGeometry& robot_geometry,
    const PlannerPose& robot_pose
) noexcept {
    if (!frame.isValid() ||
        !robot_geometry.isValid() ||
        !std::isfinite(geometry.resolution_m) ||
        geometry.resolution_m <= 0.0 ||
        !std::isfinite(geometry.forward_min_m) ||
        !std::isfinite(geometry.left_min_m) ||
        !std::isfinite(robot_pose.x_m) ||
        !std::isfinite(robot_pose.y_m) ||
        !std::isfinite(robot_pose.yaw_rad)) {
        return false;
    }

    const double resolution = geometry.resolution_m;
    const double half_length = robot_geometry.halfLength();
    const double half_width = robot_geometry.halfWidth();

    const double cos_yaw = std::cos(robot_pose.yaw_rad);
    const double sin_yaw = std::sin(robot_pose.yaw_rad);

    const int min_x = static_cast<int>(
        std::floor(
            (robot_pose.x_m - half_length -
             geometry.forward_min_m) /
            resolution
        )
    );

    const int max_x = static_cast<int>(
        std::ceil(
            (robot_pose.x_m + half_length -
             geometry.forward_min_m) /
            resolution
        )
    );

    const int min_y = static_cast<int>(
        std::floor(
            (robot_pose.y_m - half_width -
             geometry.left_min_m) /
            resolution
        )
    );

    const int max_y = static_cast<int>(
        std::ceil(
            (robot_pose.y_m + half_width -
             geometry.left_min_m) /
            resolution
        )
    );

    for (int gx = min_x; gx <= max_x; ++gx) {
        if (gx < 0 ||
            gx >= static_cast<int>(frame.height)) {
            return false;
        }

        const double forward =
            geometry.forward_min_m +
            (static_cast<double>(gx) + 0.5) *
                resolution;

        for (int gy = min_y; gy <= max_y; ++gy) {
            if (gy < 0 ||
                gy >= static_cast<int>(frame.width)) {
                return false;
            }

            const double left =
                geometry.left_min_m +
                (static_cast<double>(gy) + 0.5) *
                    resolution;

            const double dx =
                forward - robot_pose.x_m;

            const double dy =
                left - robot_pose.y_m;

            const double robot_x =
                cos_yaw * dx +
                sin_yaw * dy;

            const double robot_y =
                -sin_yaw * dx +
                cos_yaw * dy;

            if (std::abs(robot_x) <= half_length &&
                std::abs(robot_y) <= half_width) {

                const std::size_t index =
                    static_cast<std::size_t>(gx) *
                        frame.width +
                    static_cast<std::size_t>(gy);

                if (frame.observed[index] == 0) {
                    return false;
                }
            }
        }
    }

    return true;
}

}  // namespace

bool UgvRuntimeConfig::isValid() const noexcept {
    return planner_config.robot_config.isValid() &&
           controller_config.isValid() &&
           std::isfinite(robot_pose.x_m) &&
           std::isfinite(robot_pose.y_m) &&
           std::isfinite(robot_pose.yaw_rad) &&
           std::isfinite(goal.x_m) &&
           std::isfinite(goal.y_m) &&
           std::isfinite(map_geometry.resolution_m) &&
           map_geometry.resolution_m > 0.0;
}

UgvRuntime::UgvRuntime(const UgvRuntimeConfig& config)
    : config_(config),
      planner_(config.planner_config),
      controller_(config.controller_config) {
}

bool UgvRuntime::open() {
    if (!config_.isValid()) {
        return false;
    }

    if (!map_transport_.open()) {
        return false;
    }

    opened_ = true;
    return true;
}

bool UgvRuntime::processMap(
    const MetricMapFrame& frame,
    UgvRuntimeOutput& output
) {
    output = {};

    if (!frame.isValid()) {
        output.safety = SafetyCommandGate::apply(
            PlannerCommand{},
            false,
            false
        );
        return false;
    }

    CostMap cost_map(
        static_cast<int>(frame.width),
        static_cast<int>(frame.height)
    );

    if (!MetricMapCostMapAdapter::convert(
            frame,
            config_.map_geometry,
            cost_map)) {
        output.safety = SafetyCommandGate::apply(
            PlannerCommand{},
            false,
            false
        );
        return false;
    }

    output.map_valid = true;

    output.robot_footprint_observed =
        isRobotFootprintObserved(
            frame,
            config_.map_geometry,
            config_.planner_config
                .robot_config.geometry,
            config_.robot_pose
        );

    if (!output.robot_footprint_observed) {
        output.safety = SafetyCommandGate::apply(
            PlannerCommand{},
            true,
            false
        );
        return true;
    }

    output.path = planner_.planPath(
        cost_map,
        config_.robot_pose,
        config_.goal
    );

    if (!output.path.valid ||
        output.path.empty()) {
        output.safety = SafetyCommandGate::apply(
            PlannerCommand{},
            true,
            false
        );
        return true;
    }

    const PlannerCommand command =
        controller_.compute(
            output.path,
            config_.robot_pose
        );

    output.safety = SafetyCommandGate::apply(
        command,
        true,
        output.path.valid
    );

    return true;
}

bool UgvRuntime::step(
    UgvRuntimeOutput& output
) {
    output = {};

    if (!opened_) {
        return false;
    }

    MetricMapFrame frame;

    if (!map_transport_.consume(frame)) {
        output.safety = SafetyCommandGate::apply(
            PlannerCommand{},
            false,
            false
        );
        return false;
    }

    return processMap(frame, output);
}

void UgvRuntime::close() {
    map_transport_.close();
    opened_ = false;
}

}  // namespace ugv