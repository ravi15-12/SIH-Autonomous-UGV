#pragma once

#include "../traversability/cost_map.hpp"

#include <cmath>

namespace ugv {

struct GridCoordinate {
    int x = 0;
    int y = 0;
};

inline bool worldToGrid(
    const CostMap& map,
    double x_m,
    double y_m,
    GridCoordinate& output
) noexcept {

    if (!std::isfinite(x_m) ||
        !std::isfinite(y_m) ||
        !std::isfinite(map.resolution()) ||
        map.resolution() <= 0.0) {
        return false;
    }

    const double local_x =
        (x_m - map.originX()) / map.resolution();

    const double local_y =
        (y_m - map.originY()) / map.resolution();

    if (!std::isfinite(local_x) ||
        !std::isfinite(local_y)) {
        return false;
    }

    const int grid_x =
        static_cast<int>(std::floor(local_x));

    const int grid_y =
        static_cast<int>(std::floor(local_y));

    if (grid_x < 0 ||
        grid_x >= map.width() ||
        grid_y < 0 ||
        grid_y >= map.height()) {
        return false;
    }

    output.x = grid_x;
    output.y = grid_y;

    return true;
}
inline bool gridToWorld(
    const CostMap& map,
    int grid_x,
    int grid_y,
    double& x_m,
    double& y_m
) noexcept {

    if (!std::isfinite(map.resolution()) ||
        map.resolution() <= 0.0) {
        return false;
    }

    if (grid_x < 0 ||
        grid_x >= map.width() ||
        grid_y < 0 ||
        grid_y >= map.height()) {
        return false;
    }

    // Return the centre of the grid cell.
    x_m =
        map.originX() +
        (static_cast<double>(grid_x) + 0.5) *
        map.resolution();

    y_m =
        map.originY() +
        (static_cast<double>(grid_y) + 0.5) *
        map.resolution();

    return std::isfinite(x_m) &&
           std::isfinite(y_m);
}

}  // namespace ugv
