#pragma once

#include "../common/robot_footprint.hpp"
#include "../traversability/cost_map.hpp"

#include <cmath>
#include <cstdint>

namespace ugv {

inline bool inflateBlockedCells(
    const CostMap& input,
    CostMap& output,
    const RobotFootprint& footprint,
    double additional_safety_margin_m,
    std::uint8_t blocked_threshold
) noexcept {

    if (input.width() <= 0 ||
        input.height() <= 0) {
        return false;
    }

    if (output.width() != input.width() ||
        output.height() != input.height()) {
        return false;
    }

    if (!footprint.isValid()) {
        return false;
    }

    if (!std::isfinite(input.resolution()) ||
        input.resolution() <= 0.0) {
        return false;
    }

    if (!std::isfinite(additional_safety_margin_m) ||
        additional_safety_margin_m < 0.0) {
        return false;
    }

    output.setResolution(input.resolution());
    output.setTimestamp(input.timestamp());
    output.setOrigin(
        input.originX(),
        input.originY());

    output.reset();

    /*
     * Conservative circular inflation.
     *
     * The bounding radius guarantees that the complete
     * rectangular robot footprint is covered.
     *
     * The additional margin accounts for uncertainty such as:
     * - localization error
     * - perception error
     * - control tracking error
     */
    const double inflation_radius_m =
        footprint.boundingRadius() +
        additional_safety_margin_m;

    if (!std::isfinite(inflation_radius_m) ||
        inflation_radius_m < 0.0) {
        return false;
    }

    const double radius_cells =
        inflation_radius_m /
        input.resolution();

    if (!std::isfinite(radius_cells)) {
        return false;
    }

    const int radius =
        static_cast<int>(
            std::ceil(radius_cells));

    for (int y = 0; y < input.height(); ++y) {
        for (int x = 0; x < input.width(); ++x) {

            const std::uint8_t source_cost =
                input.getCost(x, y);

            if (source_cost >= blocked_threshold) {
                output.setCost(
                    x, y, blocked_threshold);
            }
        }
    }

    for (int y = 0; y < input.height(); ++y) {
        for (int x = 0; x < input.width(); ++x) {

            if (input.getCost(x, y) <
                blocked_threshold) {
                continue;
            }

            for (int dy = -radius;
                 dy <= radius;
                 ++dy) {

                for (int dx = -radius;
                     dx <= radius;
                     ++dx) {

                    const double distance_m =
                        std::hypot(
                            static_cast<double>(dx),
                            static_cast<double>(dy)) *
                        input.resolution();

                    if (distance_m >
                        inflation_radius_m) {
                        continue;
                    }

                    const int nx = x + dx;
                    const int ny = y + dy;

                    if (nx < 0 ||
                        nx >= input.width() ||
                        ny < 0 ||
                        ny >= input.height()) {
                        continue;
                    }

                    output.setCost(
                        nx,
                        ny,
                        blocked_threshold);
                }
            }
        }
    }

    return true;
}

}  // namespace ugv