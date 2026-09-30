#include "../planning/local_planner.hpp"
#include "../planning/grid_coordinates.hpp"

#include <cassert>
#include <cmath>
#include <limits>

int main() {
    ugv::PlannerConfig config;
    ugv::LocalPlanner planner(config);

    ugv::CostMap map(20, 20);
    map.setResolution(0.05);
    map.setTimestamp(1'000'000'000ULL);
    map.setOrigin(0.0, 0.0);

    ugv::PlannerPose pose;
    ugv::PlannerGoal goal;

    // A clear map should produce a valid path.
    {
        goal.x_m = 0.40;
        goal.y_m = 0.40;

        const auto path =
            planner.planPath(map, pose, goal);

        assert(path.valid);
        assert(!path.empty());
        assert(path.size() > 1);
        assert(path.length_m > 0.0);
        assert(path.maximum_cost <= 30.0);
        assert(path.minimum_clearance_m >= 0.0);
        assert(std::isinf(path.minimum_clearance_m));

        goal.x_m = 0.0;
        goal.y_m = 0.0;
    }

    // Current placeholder planner must never issue a command.
    {
        const auto command =
            planner.compute(map, pose, goal);

        assert(!command.valid);
        assert(command.linear_velocity_mps == 0.0);
        assert(command.angular_velocity_rps == 0.0);
    }

    // Reject invalid path-cost weight.
    {
        ugv::PlannerConfig invalid_config = config;
        invalid_config.path_cost_weight = -1.0;

        ugv::LocalPlanner invalid_planner(invalid_config);

        const auto command =
            invalid_planner.compute(map, pose, goal);

        assert(!command.valid);
    }

    // Reject zero search-node limit.
    {
        ugv::PlannerConfig invalid_config = config;
        invalid_config.maximum_search_nodes = 0;

        ugv::LocalPlanner invalid_planner(invalid_config);

        const auto command =
            invalid_planner.compute(map, pose, goal);

        assert(!command.valid);
    }

    // A blocked wall with a wide opening should produce
    // a valid path through the opening even after safety inflation.
    {
        ugv::CostMap obstacle_map(20, 20);

        obstacle_map.setResolution(0.05);
        obstacle_map.setTimestamp(1'000'000'000ULL);
        obstacle_map.setOrigin(-0.50, -0.50);

        // Wall at x = 10.
        //
        // Inflation is derived from the configured robot footprint.
        //
// With the current provisional default geometry, the
// conservative bounding radius is approximately 0.25 m,
// which corresponds to about 5 cells at 0.05 m/cell.
//
// Leave a sufficiently wide opening so the configured
// robot footprint can physically pass through it.
        for (int y = 0; y < 20; ++y) {
            if (y < 2 || y > 17) {
                obstacle_map.setCost(10, y, 100);
            }
        }

        ugv::PlannerPose start_pose;
        start_pose.x_m = -0.20;
        start_pose.y_m = -0.20;

        ugv::PlannerGoal wall_goal;
        wall_goal.x_m = 0.40;
        wall_goal.y_m = 0.20;

        const auto path =
            planner.planPath(
                obstacle_map,
                start_pose,
                wall_goal);

        assert(path.valid);
        assert(!path.empty());
        assert(path.size() > 1);

        // The returned path must not contain blocked cells
        // in the original perception cost map.
        for (const auto& point : path.points) {
            ugv::GridCoordinate cell;

            assert(ugv::worldToGrid(
                obstacle_map,
                point.x_m,
                point.y_m,
                cell));

            assert(obstacle_map.getCost(
                cell.x,
                cell.y) < 100);
        }
    }

    // Reject NaN robot position.
    {
        pose.x_m =
            std::numeric_limits<double>::quiet_NaN();

        const auto command =
            planner.compute(map, pose, goal);

        assert(!command.valid);

        pose.x_m = 0.0;
    }

    // Reject NaN goal position.
    {
        goal.x_m =
            std::numeric_limits<double>::quiet_NaN();

        const auto command =
            planner.compute(map, pose, goal);

        assert(!command.valid);

        goal.x_m = 0.0;
    }

    // Reject a goal beyond the configured local-planning range.
    {
        goal.x_m =
            config.max_goal_distance_m + 1.0;

        const auto command =
            planner.compute(map, pose, goal);

        assert(!command.valid);

        goal.x_m = 0.0;
    }

    // Reject invalid map resolution.
    {
        ugv::CostMap invalid_map(20, 20);

        const auto command =
            planner.compute(invalid_map, pose, goal);

        assert(!command.valid);
    }

    return 0;
}