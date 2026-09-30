#include <limits>
#include "../common/robot_footprint.hpp"

#include <cassert>
#include <cmath>

using namespace ugv;

int main() {

    RobotGeometry geometry;

    geometry.length_m = 0.40;
    geometry.width_m = 0.30;
    geometry.wheel_base_m = 0.30;
    geometry.track_width_m = 0.30;
    geometry.front_overhang_m = 0.05;
    geometry.rear_overhang_m = 0.05;

    RobotFootprint footprint(geometry);

    // Geometry must be valid.
    assert(footprint.isValid());

    // Half dimensions.
    assert(std::abs(
        footprint.halfLength() - 0.20) < 1e-9);

    assert(std::abs(
        footprint.halfWidth() - 0.15) < 1e-9);

    // Bounding radius.
    const double expected_radius =
        std::hypot(0.20, 0.15);

    assert(std::abs(
        footprint.boundingRadius() -
        expected_radius) < 1e-9);

    // Center must be inside.
    assert(footprint.containsPoint(0.0, 0.0));

    // Boundary points are inside.
    assert(footprint.containsPoint(0.20, 0.15));
    assert(footprint.containsPoint(-0.20, -0.15));

    // Points outside the footprint must be rejected.
    assert(!footprint.containsPoint(0.21, 0.0));
    assert(!footprint.containsPoint(0.0, 0.16));
    assert(!footprint.containsPoint(-0.21, 0.0));

    // Non-finite points must be rejected.
    assert(!footprint.containsPoint(
        std::numeric_limits<double>::quiet_NaN(),
        0.0));

    assert(!footprint.containsPoint(
        0.0,
        std::numeric_limits<double>::infinity()));

    // Invalid geometry must invalidate the footprint.
    RobotGeometry invalid_geometry;
    invalid_geometry.width_m = -1.0;

    RobotFootprint invalid_footprint(
        invalid_geometry);

    assert(!invalid_footprint.isValid());
    assert(invalid_footprint.boundingRadius() == 0.0);
    assert(!invalid_footprint.containsPoint(0.0, 0.0));

    return 0;
}