#pragma once

#include "../traversability/cost_map.hpp"

#include <cstdint>
#include <limits>

namespace ugv {

inline double calculateCellClearance(
    const CostMap& map,
    int cell_x,
    int cell_y,
    std::uint8_t blocked_threshold
) noexcept {

    if (cell_x < 0 ||
        cell_x >= map.width() ||
        cell_y < 0 ||
        cell_y >= map.height()) {
        return 0.0;
    }

    double minimum_distance_cells =
        std::numeric_limits<double>::infinity();

    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {

            if (map.getCost(x, y) < blocked_threshold) {
                continue;
            }

            const double dx =
                static_cast<double>(x - cell_x);

            const double dy =
                static_cast<double>(y - cell_y);

            const double distance =
                std::hypot(dx, dy);

            if (distance < minimum_distance_cells) {
                minimum_distance_cells = distance;
            }
        }
    }

    if (!std::isfinite(minimum_distance_cells)) {
        return std::numeric_limits<double>::infinity();
    }

    return minimum_distance_cells * map.resolution();
}

}  // namespace ugv
