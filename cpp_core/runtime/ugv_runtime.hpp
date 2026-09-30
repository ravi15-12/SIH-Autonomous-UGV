#pragma once

#include "../transport/synchronized_metric_map_transport.hpp"
#include "../../mapping/metric_map_costmap_adapter.hpp"
#include "../planning/local_planner.hpp"
#include "../control/motion_controller.hpp"
#include "../control/safety_command.hpp"

namespace ugv {

struct UgvRuntimeConfig {
    MetricMapGeometry map_geometry{};
    PlannerConfig planner_config{};
    MotionControllerConfig controller_config{};
    PlannerPose robot_pose{};
    PlannerGoal goal{};

    bool isValid() const noexcept;
};

struct UgvRuntimeOutput {
    SafetyCommand safety{};
    Path path{};
    bool map_valid = false;
    bool robot_footprint_observed = false;

};

class UgvRuntime {
public:
    explicit UgvRuntime(const UgvRuntimeConfig& config);

    bool open();
    bool step(UgvRuntimeOutput& output);
    bool processMap(const MetricMapFrame& frame, UgvRuntimeOutput& output);
    void close();

private:
    UgvRuntimeConfig config_;
    LocalPlanner planner_;
    MotionController controller_;
    SynchronizedMetricMapTransport map_transport_;
    bool opened_ = false;
};

}  // namespace ugv
