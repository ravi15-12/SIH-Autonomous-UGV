#include "../transport/synchronized_perception_transport.hpp"
#include "../traversability/cost_map.hpp"
#include "../traversability/traversability_config.hpp"
#include "../traversability/traversability_engine.hpp"
#include "../planning/local_planner.hpp"

#include <cmath>
#include <iostream>

int main() {
    ugv::SynchronizedPerceptionTransport transport;

    if (!transport.open(
            "ugv_perception_frame",
            "ugv_perception_ready",
            "ugv_perception_free")) {
        std::cerr
            << "FAILED: unable to open synchronized transport\n";
        return 1;
    }

    ugv::PerceptionFrame perception;

    if (!transport.consume(perception)) {
        std::cerr
            << "FAILED: unable to consume perception frame\n";
        transport.close();
        return 1;
    }

    if (!perception.isValid()) {
        std::cerr
            << "FAILED: invalid perception frame\n";
        transport.close();
        return 1;
    }

    ugv::TraversabilityConfig traversability_config;

    ugv::TraversabilityEngine traversability_engine(
        traversability_config);

    ugv::CostMap cost_map(
        perception.width,
        perception.height);

    if (!traversability_engine.compute(
            perception,
            perception.timestamp_ns,
            cost_map)) {
        std::cerr
            << "FAILED: traversability computation failed\n";
        transport.close();
        return 1;
    }

    ugv::PlannerConfig planner_config;

    ugv::LocalPlanner planner(
        planner_config);

    /*
     * The current CostMap origin/resolution define the local
     * planning frame. Use a nearby goal inside the generated map.
     *
     * This is an integration test only; camera-to-ground
     * projection and physical pose estimation are not yet implemented.
     */
    ugv::PlannerPose robot_pose;
    robot_pose.x_m = 0.0;
    robot_pose.y_m = 0.0;
    robot_pose.yaw_rad = 0.0;

    ugv::PlannerGoal goal;
    goal.x_m = 0.5;
    goal.y_m = 0.0;

    const auto path =
        planner.planPath(
            cost_map,
            robot_pose,
            goal);

    transport.close();

    if (!path.valid) {
        std::cerr
            << "FAILED: planner could not produce a valid path\n";
        return 1;
    }

    if (path.empty()) {
        std::cerr
            << "FAILED: planner returned an empty path\n";
        return 1;
    }

    if (!std::isfinite(path.length_m) ||
        path.length_m < 0.0) {
        std::cerr
            << "FAILED: invalid path length\n";
        return 1;
    }

    if (!std::isfinite(path.maximum_cost) ||
        path.maximum_cost < 0.0) {
        std::cerr
            << "FAILED: invalid maximum path cost\n";
        return 1;
    }

    std::cout
        << "LIVE PERCEPTION -> TRAVERSABILITY -> PLANNER: PASS\n"
        << "Perception: "
        << perception.width
        << " x "
        << perception.height
        << "\n"
        << "Path points: "
        << path.points.size()
        << "\n"
        << "Path length: "
        << path.length_m
        << " m\n"
        << "Maximum cost: "
        << path.maximum_cost
        << "\n";

    return 0;
}
