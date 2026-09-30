#pragma once

#include <array>

namespace ugv {

struct GridNeighbor {
    int dx = 0;
    int dy = 0;
    double movement_cost = 0.0;
};

inline constexpr std::array<GridNeighbor, 8>
gridNeighbors() noexcept {
    return {{
        { 1,  0, 1.0},
        {-1,  0, 1.0},
        { 0,  1, 1.0},
        { 0, -1, 1.0},

        { 1,  1, 1.4142135623730951},
        { 1, -1, 1.4142135623730951},
        {-1,  1, 1.4142135623730951},
        {-1, -1, 1.4142135623730951}
    }};
}

}  // namespace ugv
