#pragma once

#include "../traversability/cost_map.hpp"

#include <cstdint>

namespace ugv {

inline bool isTraversableCell(
    const CostMap& map,
    int x,
    int y,
    std::uint8_t traversable_threshold,
    std::uint8_t blocked_threshold
) noexcept {

    if (x < 0 ||
        x >= map.width() ||
        y < 0 ||
        y >= map.height()) {
        return false;
    }

    if (traversable_threshold >= blocked_threshold) {
        return false;
    }

    const std::uint8_t cost =
        map.getCost(x, y);

    return cost <= traversable_threshold;
}

inline bool isValidMove(
    const CostMap& map,
    int from_x,
    int from_y,
    int to_x,
    int to_y,
    std::uint8_t traversable_threshold,
    std::uint8_t blocked_threshold
) noexcept {

    const int dx = to_x - from_x;
    const int dy = to_y - from_y;

    // Only adjacent cells are valid moves.
    if (dx < -1 || dx > 1 ||
        dy < -1 || dy > 1 ||
        (dx == 0 && dy == 0)) {
        return false;
    }

    if (!isTraversableCell(
            map,
            to_x,
            to_y,
            traversable_threshold,
            blocked_threshold)) {
        return false;
    }

    // For diagonal movement, both orthogonal
    // neighbouring cells must also be traversable.
    if (dx != 0 && dy != 0) {

        if (!isTraversableCell(
                map,
                from_x + dx,
                from_y,
                traversable_threshold,
                blocked_threshold)) {
            return false;
        }

        if (!isTraversableCell(
                map,
                from_x,
                from_y + dy,
                traversable_threshold,
                blocked_threshold)) {
            return false;
        }
    }

    return true;
}

}  // namespace ugv