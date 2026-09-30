#pragma once

#include <cmath>

namespace ugv {

inline double euclideanHeuristic(
    int x,
    int y,
    int goal_x,
    int goal_y
) noexcept {

    const double dx =
        static_cast<double>(goal_x - x);

    const double dy =
        static_cast<double>(goal_y - y);

    const double distance =
        std::hypot(dx, dy);

    if (!std::isfinite(distance)) {
        return 0.0;
    }

    return distance;
}

}  // namespace ugv
