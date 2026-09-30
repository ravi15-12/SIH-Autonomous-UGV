#include "../common/robot_geometry.hpp"

#include <cassert>
#include <cmath>
#include <limits>

using namespace ugv;

int main() {

    // Default geometry must be valid.
    {
        RobotGeometry geometry;
        assert(geometry.isValid());
    }

    // Custom robot geometry must also work.
    {
        RobotGeometry geometry;

        geometry.length_m = 0.60;
        geometry.width_m = 0.45;
        geometry.wheel_base_m = 0.40;
        geometry.track_width_m = 0.35;
        geometry.front_overhang_m = 0.10;
        geometry.rear_overhang_m = 0.10;

        assert(geometry.isValid());
    }

    // Half dimensions.
    {
        RobotGeometry geometry;

        geometry.length_m = 0.60;
        geometry.width_m = 0.40;

        assert(std::abs(
            geometry.halfLength() - 0.30) < 1e-9);

        assert(std::abs(
            geometry.halfWidth() - 0.20) < 1e-9);
    }

    // Bounding radius.
    {
        RobotGeometry geometry;

        geometry.length_m = 0.40;
        geometry.width_m = 0.30;

        const double expected =
            std::hypot(0.20, 0.15);

        assert(std::abs(
            geometry.boundingRadius() - expected) < 1e-9);
    }

    // Invalid length.
    {
        RobotGeometry geometry;
        geometry.length_m = 0.0;

        assert(!geometry.isValid());
    }

    // Invalid width.
    {
        RobotGeometry geometry;
        geometry.width_m = -0.1;

        assert(!geometry.isValid());
    }

    // Wheelbase cannot exceed total length.
    {
        RobotGeometry geometry;
        geometry.wheel_base_m = 0.50;
        geometry.length_m = 0.40;

        assert(!geometry.isValid());
    }

    // Track width cannot exceed total width.
    {
        RobotGeometry geometry;
        geometry.track_width_m = 0.40;
        geometry.width_m = 0.30;

        assert(!geometry.isValid());
    }

    // Negative overhang is invalid.
    {
        RobotGeometry geometry;
        geometry.front_overhang_m = -0.01;

        assert(!geometry.isValid());
    }

    // Non-finite values are invalid.
    {
        RobotGeometry geometry;
        geometry.length_m =
            std::numeric_limits<double>::quiet_NaN();

        assert(!geometry.isValid());
    }

    {
        RobotGeometry geometry;
        geometry.width_m =
            std::numeric_limits<double>::infinity();

        assert(!geometry.isValid());
    }

    return 0;
}