#include "local_planner.hpp"

#include "grid_coordinates.hpp"
#include "grid_distance_transform.hpp"
#include "grid_heuristic.hpp"
#include "grid_inflation.hpp"
#include "grid_neighbors.hpp"
#include "grid_search.hpp"
#include "search_queue.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <vector>

namespace ugv {

LocalPlanner::LocalPlanner(const PlannerConfig& config)
    : config_(config) {}

PlannerCommand LocalPlanner::compute(
    const CostMap& cost_map,
    const PlannerPose& robot_pose,
    const PlannerGoal& goal
) const noexcept {

    const Path path =
        planPath(cost_map, robot_pose, goal);

    if (!path.valid || path.empty()) {
        return {};
    }

    // Motion-control generation will be implemented separately.
    // Never generate a velocity command from an unvalidated path.
    return {};
}

Path LocalPlanner::planPath(
    const CostMap& cost_map,
    const PlannerPose& robot_pose,
    const PlannerGoal& goal
) const noexcept {

    Path result;

    if (!isInputValid(
            cost_map,
            robot_pose,
            goal)) {
        return result;
    }

    /*
     * Create a planning map from the perception-derived
     * cost map.
     *
     * Obstacles are inflated using the configured physical
     * robot footprint plus an additional safety margin.
     */
    CostMap planning_map(
        cost_map.width(),
        cost_map.height());

    const RobotFootprint robot_footprint(
        config_.robot_config.geometry);

    if (!inflateBlockedCells(
            cost_map,
            planning_map,
            robot_footprint,
            config_.robot_config.additional_safety_margin_m,
            config_.blocked_threshold)) {
        return result;
    }

    /*
     * Precompute clearance from every cell to the nearest
     * blocked cell in the inflated planning map.
     */
    GridDistanceTransform distance_transform;

    if (!distance_transform.compute(
            planning_map,
            config_.blocked_threshold)) {
        return result;
    }

    GridCoordinate start;
    GridCoordinate target;

    if (!worldToGrid(
            planning_map,
            robot_pose.x_m,
            robot_pose.y_m,
            start)) {
        return result;
    }

    if (!worldToGrid(
            planning_map,
            goal.x_m,
            goal.y_m,
            target)) {
        return result;
    }

    /*
     * The robot's current cell and goal cell must both be
     * traversable after robot-footprint inflation.
     */
    if (!isTraversableCell(
            planning_map,
            start.x,
            start.y,
            config_.traversable_threshold,
            config_.blocked_threshold)) {
        return result;
    }

    if (!isTraversableCell(
            planning_map,
            target.x,
            target.y,
            config_.traversable_threshold,
            config_.blocked_threshold)) {
        return result;
    }

    /*
     * Robot is already in the goal cell.
     */
    if (start.x == target.x &&
        start.y == target.y) {

        double x_m = 0.0;
        double y_m = 0.0;

        if (!gridToWorld(
                cost_map,
                start.x,
                start.y,
                x_m,
                y_m)) {
            return result;
        }

        result.points.push_back({
            x_m,
            y_m
        });

        result.length_m = 0.0;

        result.maximum_cost =
            static_cast<double>(
                cost_map.getCost(
                    start.x,
                    start.y));

        result.minimum_clearance_m =
            distance_transform.clearanceMeters(
                planning_map,
                start.x,
                start.y);

        result.valid = true;

        return result;
    }

    const std::size_t width =
        static_cast<std::size_t>(
            planning_map.width());

    const std::size_t height =
        static_cast<std::size_t>(
            planning_map.height());

    const std::size_t node_count =
        width * height;

    if (node_count == 0) {
        return result;
    }

    std::vector<double> g_cost(
        node_count,
        std::numeric_limits<double>::infinity());

    std::vector<int> parent(
        node_count,
        -1);

    std::vector<bool> closed(
        node_count,
        false);

    const auto index =
        [width](int x, int y) -> std::size_t {
            return
                static_cast<std::size_t>(y) * width +
                static_cast<std::size_t>(x);
        };

    const std::size_t start_index =
        index(start.x, start.y);

    const std::size_t target_index =
        index(target.x, target.y);

    std::priority_queue<
        SearchQueueEntry,
        std::vector<SearchQueueEntry>,
        SearchQueueEntryCompare
    > open;

    g_cost[start_index] = 0.0;

    open.push({
        start.x,
        start.y,
        euclideanHeuristic(
            start.x,
            start.y,
            target.x,
            target.y)
    });

    std::size_t expanded_nodes = 0;

    bool found = false;

    while (!open.empty()) {

        if (expanded_nodes >=
            config_.maximum_search_nodes) {
            return result;
        }

        const SearchQueueEntry current =
            open.top();

        open.pop();

        const std::size_t current_index =
            index(
                current.x,
                current.y);

        if (closed[current_index]) {
            continue;
        }

        closed[current_index] = true;
        ++expanded_nodes;

        if (current.x == target.x &&
            current.y == target.y) {

            found = true;
            break;
        }

        for (const auto& neighbor :
             gridNeighbors()) {

            const int next_x =
                current.x + neighbor.dx;

            const int next_y =
                current.y + neighbor.dy;

            if (!isValidMove(
                    planning_map,
                    current.x,
                    current.y,
                    next_x,
                    next_y,
                    config_.traversable_threshold,
                    config_.blocked_threshold)) {
                continue;
            }

            const std::size_t next_index =
                index(
                    next_x,
                    next_y);

            if (closed[next_index]) {
                continue;
            }

            const double cell_cost =
                static_cast<double>(
                    planning_map.getCost(
                        next_x,
                        next_y));

            const double normalized_cell_cost =
                cell_cost / 100.0;

            const double movement_cost =
                config_.path_length_weight *
                    neighbor.movement_cost +
                config_.path_cost_weight *
                    neighbor.movement_cost *
                    normalized_cell_cost;

            const double tentative_g =
                g_cost[current_index] +
                movement_cost;

            if (tentative_g >=
                g_cost[next_index]) {
                continue;
            }

            g_cost[next_index] =
                tentative_g;

            parent[next_index] =
                static_cast<int>(
                    current_index);

            const double heuristic =
                euclideanHeuristic(
                    next_x,
                    next_y,
                    target.x,
                    target.y);

            open.push({
                next_x,
                next_y,
                tentative_g + heuristic
            });
        }
    }

    if (!found) {
        return result;
    }

    /*
     * Reconstruct the grid path from target to start.
     */
    std::vector<GridCoordinate> reverse_path;

    std::size_t current_index =
        target_index;

    while (true) {

        const int current_x =
            static_cast<int>(
                current_index % width);

        const int current_y =
            static_cast<int>(
                current_index / width);

        reverse_path.push_back({
            current_x,
            current_y
        });

        if (current_index == start_index) {
            break;
        }

        const int parent_index =
            parent[current_index];

        if (parent_index < 0) {
            return Path{};
        }

        current_index =
            static_cast<std::size_t>(
                parent_index);

        if (reverse_path.size() >
            config_.maximum_path_points) {
            return Path{};
        }
    }

    std::reverse(
        reverse_path.begin(),
        reverse_path.end());

    if (reverse_path.empty()) {
        return result;
    }

    result.points.reserve(
        reverse_path.size());

    double total_length_m = 0.0;
    double maximum_cost = 0.0;

    double minimum_clearance_m =
        std::numeric_limits<double>::infinity();

    for (std::size_t i = 0;
         i < reverse_path.size();
         ++i) {

        const auto& cell =
            reverse_path[i];

        double x_m = 0.0;
        double y_m = 0.0;

        if (!gridToWorld(
                cost_map,
                cell.x,
                cell.y,
                x_m,
                y_m)) {
            return Path{};
        }

        result.points.push_back({
            x_m,
            y_m
        });

        maximum_cost =
            std::max(
                maximum_cost,
                static_cast<double>(
                    cost_map.getCost(
                        cell.x,
                        cell.y)));

        const double clearance_m =
            distance_transform.clearanceMeters(
                planning_map,
                cell.x,
                cell.y);

        if (std::isfinite(clearance_m)) {
            minimum_clearance_m =
                std::min(
                    minimum_clearance_m,
                    clearance_m);
        }

        if (i > 0) {

            const double dx =
                result.points[i].x_m -
                result.points[i - 1].x_m;

            const double dy =
                result.points[i].y_m -
                result.points[i - 1].y_m;

            const double segment =
                std::hypot(
                    dx,
                    dy);

            if (!std::isfinite(segment)) {
                return Path{};
            }

            total_length_m += segment;
        }
    }

    if (!std::isfinite(total_length_m) ||
        !std::isfinite(maximum_cost)) {
        return Path{};
    }

    result.length_m =
        total_length_m;

    result.maximum_cost =
        maximum_cost;

    /*
     * Clearance is measured on the inflated planning map.
     *
     * This means the reported value represents clearance
     * after accounting for the configured robot footprint
     * and additional safety margin.
     */
    result.minimum_clearance_m =
        minimum_clearance_m;

    result.valid = true;

    return result;
}

bool LocalPlanner::isInputValid(
    const CostMap& cost_map,
    const PlannerPose& robot_pose,
    const PlannerGoal& goal
) const noexcept {

    if (cost_map.width() <= 0 ||
        cost_map.height() <= 0) {
        return false;
    }

    if (!std::isfinite(
            cost_map.resolution()) ||
        cost_map.resolution() <= 0.0) {
        return false;
    }

    if (!std::isfinite(
            robot_pose.x_m) ||
        !std::isfinite(
            robot_pose.y_m) ||
        !std::isfinite(
            robot_pose.yaw_rad)) {
        return false;
    }

    if (!std::isfinite(
            goal.x_m) ||
        !std::isfinite(
            goal.y_m)) {
        return false;
    }

    if (!std::isfinite(
            config_.max_goal_distance_m) ||
        config_.max_goal_distance_m <= 0.0) {
        return false;
    }

    if (!std::isfinite(
            config_.minimum_clearance_m) ||
        config_.minimum_clearance_m < 0.0) {
        return false;
    }

    if (!std::isfinite(
            config_.robot_config.additional_safety_margin_m) ||
        config_.robot_config.additional_safety_margin_m < 0.0) {
        return false;
    }

    if (!config_.robot_config.isValid()) {
        return false;
    }

    if (config_.traversable_threshold >=
        config_.blocked_threshold) {
        return false;
    }

    if (!std::isfinite(
            config_.path_cost_weight) ||
        !std::isfinite(
            config_.path_length_weight) ||
        !std::isfinite(
            config_.clearance_weight) ||
        config_.path_cost_weight < 0.0 ||
        config_.path_length_weight < 0.0 ||
        config_.clearance_weight < 0.0) {
        return false;
    }

    if (config_.maximum_path_points == 0 ||
        config_.maximum_search_nodes == 0) {
        return false;
    }

    const double dx =
        goal.x_m -
        robot_pose.x_m;

    const double dy =
        goal.y_m -
        robot_pose.y_m;

    const double distance_m =
        std::hypot(
            dx,
            dy);

    if (!std::isfinite(distance_m) ||
        distance_m >
            config_.max_goal_distance_m) {
        return false;
    }

    return true;
}

}  // namespace ugv