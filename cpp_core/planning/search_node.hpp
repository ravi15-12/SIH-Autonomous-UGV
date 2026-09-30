#pragma once

#include <cstddef>
#include <limits>

namespace ugv {

struct SearchNode {
    int x = 0;
    int y = 0;

    double cost_from_start = 0.0;
    double estimated_total_cost = 0.0;

    int parent_x = -1;
    int parent_y = -1;

    bool has_parent = false;
    bool closed = false;

    static constexpr double infinity() noexcept {
        return std::numeric_limits<double>::infinity();
    };
};

}  // namespace ugv
