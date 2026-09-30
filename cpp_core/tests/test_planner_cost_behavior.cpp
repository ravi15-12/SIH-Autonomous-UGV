#include "../planning/local_planner.hpp"
#include "../planning/grid_coordinates.hpp"

#include <cassert>
#include <cmath>

int main() {
    /*
     * The test creates two possible routes:
     *
     *   Route A: shorter but higher cost
     *   Route B: slightly longer but lower cost
     *
     * A planner with a higher path-cost weight should prefer
     * the lower-cost route.
     */

    ugv::CostMap map(20, 20);

    map.setResolution(0.05);
    map.setTimestamp(1'000'000'000ULL);
    map.setOrigin(0.0, 0.0);

    /*
     * Create a higher-cost band in the middle of the map.
     *
     * Cost 25 is still below the traversable threshold of 30,
     * so the planner is allowed to travel through it.
     */
    for (int x = 4; x <= 15; ++x) {
        for (int y = 8; y <= 11; ++y) {
            map.setCost(x, y, 25);
        }
    }

    ugv::PlannerPose pose;
    pose.x_m = 0.25;
    pose.y_m = 0.45;

    ugv::PlannerGoal goal;
    goal.x_m = 0.75;
    goal.y_m = 0.45;

    /*
     * Planner A strongly prioritizes path length.
     */
    ugv::PlannerConfig short_config;
    short_config.path_length_weight = 5.0;
    short_config.path_cost_weight = 0.1;

    ugv::LocalPlanner short_planner(short_config);

    const auto short_path =
        short_planner.planPath(
            map,
            pose,
            goal);

    assert(short_path.valid);
    assert(!short_path.empty());

    /*
     * Planner B strongly prioritizes traversal cost.
     */
    ugv::PlannerConfig safe_config;
    safe_config.path_length_weight = 1.0;
    safe_config.path_cost_weight = 10.0;

    ugv::LocalPlanner safe_planner(safe_config);

    const auto safe_path =
        safe_planner.planPath(
            map,
            pose,
            goal);

    assert(safe_path.valid);
    assert(!safe_path.empty());

    /*
     * Both planners must reach the goal.
     */
    assert(short_path.length_m > 0.0);
    assert(safe_path.length_m > 0.0);

    /*
     * The cost-aware planner should not have a higher maximum
     * path cost than the strongly length-biased planner.
     */
    assert(
        safe_path.maximum_cost <=
        short_path.maximum_cost
    );

    /*
     * Every returned path point must correspond to a traversable
     * cell in the original map.
     */
    for (const auto& point : safe_path.points) {
        ugv::GridCoordinate cell;

        assert(ugv::worldToGrid(
            map,
            point.x_m,
            point.y_m,
            cell));

        assert(
            map.getCost(cell.x, cell.y) <
            safe_config.blocked_threshold
        );
    }

    return 0;
}