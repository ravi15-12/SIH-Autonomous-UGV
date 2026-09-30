#pragma once

#include "robot_geometry.hpp"

#include <cmath>

namespace ugv {

/*
 * RobotFootprint provides geometry-derived safety information
 * from the configured robot dimensions.
 *
 * The footprint itself is robot-independent.
 * Robot-specific dimensions come only from RobotGeometry.
 */
class RobotFootprint {
public:

    explicit RobotFootprint(
        const RobotGeometry& geometry)
        : geometry_(geometry) {}

    bool isValid() const noexcept {
        return geometry_.isValid();
    }

    /*
     * Conservative circular footprint radius.
     *
     * Any point within this radius from the robot reference
     * point is guaranteed to lie inside the rectangular
     * robot footprint.
     */
    double boundingRadius() const noexcept {
        if (!isValid()) {
            return 0.0;
        }

        return geometry_.boundingRadius();
    }

    /*
     * Half-length of the robot body.
     */
    double halfLength() const noexcept {
        if (!isValid()) {
            return 0.0;
        }

        return geometry_.halfLength();
    }

    /*
     * Half-width of the robot body.
     */
    double halfWidth() const noexcept {
        if (!isValid()) {
            return 0.0;
        }

        return geometry_.halfWidth();
    }

    /*
     * Check whether a point expressed in the robot's local
     * coordinate frame lies inside the rectangular footprint.
     *
     * x: forward/backward direction
     * y: left/right direction
     */
    bool containsPoint(
        double x_m,
        double y_m
    ) const noexcept {

        if (!isValid() ||
            !std::isfinite(x_m) ||
            !std::isfinite(y_m)) {
            return false;
        }

        return std::abs(x_m) <= halfLength() &&
               std::abs(y_m) <= halfWidth();
    }

private:
    RobotGeometry geometry_;
};

}  // namespace ugv