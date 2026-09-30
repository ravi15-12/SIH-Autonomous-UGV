#pragma once

#include <cstddef>
#include <vector>

namespace ugv {

struct PathPoint {
    double x_m = 0.0;
    double y_m = 0.0;
};

struct Path {
    std::vector<PathPoint> points;

    // Total geometric length of the path, metres.
    double length_m = 0.0;

    // Maximum cost encountered along the path.
    double maximum_cost = 0.0;

    // Minimum clearance from blocked space, metres.
    double minimum_clearance_m = 0.0;

    // True only when the path has passed all required
    // planner safety checks.
    bool valid = false;

    bool empty() const noexcept {
        return points.empty();
    }

    std::size_t size() const noexcept {
        return points.size();
    }
};

}  // namespace ugv
