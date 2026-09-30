#pragma once

#include <cmath>

namespace ugv {

/*
 * RobotGeometry describes the physical dimensions of a UGV.
 *
 * These values are deliberately kept separate from the
 * navigation algorithms so the same software can support
 * different robot platforms.
 *
 * IMPORTANT:
 * Default values are provisional engineering values.
 * They must be replaced with measured dimensions before
 * physical deployment.
 */
struct RobotGeometry {

    // Overall physical dimensions.
    double length_m = 0.40;
    double width_m = 0.30;

    // Wheel geometry.
    double wheel_base_m = 0.30;
    double track_width_m = 0.30;

    // Distance from the wheelbase rectangle to the
    // front and rear physical boundaries of the chassis.
    double front_overhang_m = 0.05;
    double rear_overhang_m = 0.05;

    /*
     * Validate the geometry.
     *
     * Requirements:
     * - all dimensions must be finite
     * - all dimensions must be positive
     * - wheelbase and track width must fit within the
     *   corresponding chassis dimensions
     * - front + rear overhang must not exceed total length
     */
    bool isValid() const noexcept {

        if (!std::isfinite(length_m) ||
            !std::isfinite(width_m) ||
            !std::isfinite(wheel_base_m) ||
            !std::isfinite(track_width_m) ||
            !std::isfinite(front_overhang_m) ||
            !std::isfinite(rear_overhang_m)) {
            return false;
        }

        if (length_m <= 0.0 ||
            width_m <= 0.0 ||
            wheel_base_m <= 0.0 ||
            track_width_m <= 0.0 ||
            front_overhang_m < 0.0 ||
            rear_overhang_m < 0.0) {
            return false;
        }

        if (wheel_base_m > length_m) {
            return false;
        }

        if (track_width_m > width_m) {
            return false;
        }

        if ((wheel_base_m + front_overhang_m +
             rear_overhang_m) > length_m) {
            return false;
        }

        return true;
    }

    /*
     * Half dimensions are useful when constructing
     * a rectangular robot footprint around its center.
     */
    double halfLength() const noexcept {
        return length_m * 0.5;
    }

    double halfWidth() const noexcept {
        return width_m * 0.5;
    }

    /*
     * Maximum distance from the robot center to any
     * corner of the rectangular footprint.
     *
     * This is useful for conservative circular
     * safety checks.
     */
    double boundingRadius() const noexcept {

        return std::hypot(
            halfLength(),
            halfWidth());
    }
};

}  // namespace ugv