#include "../traversability/cost_map.hpp"
#include "../planning/grid_inflation.hpp"
#include "../common/robot_footprint.hpp"

#include <cassert>
#include <cmath>

int main() {

    // Robot geometry used by the test.
    //
    // These dimensions are deliberately explicit so the test
    // remains independent of future project defaults.
    ugv::RobotGeometry geometry;

    geometry.length_m = 0.20;
    geometry.width_m = 0.10;
    geometry.wheel_base_m = 0.16;
    geometry.track_width_m = 0.08;
    geometry.front_overhang_m = 0.02;
    geometry.rear_overhang_m = 0.02;

    ugv::RobotFootprint footprint(geometry);

    assert(footprint.isValid());

    // 11x11 map with 0.10 m/cell resolution.
    ugv::CostMap input(11, 11);
    input.setResolution(0.10);
    input.setTimestamp(1'000'000'000ULL);
    input.setOrigin(-0.50, -0.50);

    // One blocked cell at the center.
    input.setCost(5, 5, 100);

    ugv::CostMap output(11, 11);

    /*
     * The robot bounding radius is approximately 0.071 m.
     * With zero additional safety margin, diagonal cells
     * are within the conservative circular footprint.
     *
     * Therefore this test verifies the new geometry-based
     * inflation behavior rather than the old 0.10 m radius.
     */
    const bool success =
        ugv::inflateBlockedCells(
            input,
            output,
            footprint,
            0.0,
            100);

    assert(success);

    // Metadata must be preserved.
    assert(output.resolution() == input.resolution());
    assert(output.timestamp() == input.timestamp());
    assert(output.originX() == input.originX());
    assert(output.originY() == input.originY());

    // Center remains blocked.
    assert(output.getCost(5, 5) == 100);

    // Cardinal neighbors are within the footprint radius.
    assert(output.getCost(4, 5) == 100);
    assert(output.getCost(6, 5) == 100);
    assert(output.getCost(5, 4) == 100);
    assert(output.getCost(5, 6) == 100);

    // Diagonal neighbors are also within the approximately
    // 0.071 m bounding radius when measured at 0.10 m/cell
    // only if their cell-center distance is <= radius.
    //
    // A diagonal cell is approximately 0.141 m away, so it
    // must remain unblocked.
    assert(output.getCost(4, 4) < 100);
    assert(output.getCost(4, 6) < 100);
    assert(output.getCost(6, 4) < 100);
    assert(output.getCost(6, 6) < 100);

    // Two cells away must remain unblocked.
    assert(output.getCost(3, 5) < 100);
    assert(output.getCost(7, 5) < 100);
    assert(output.getCost(5, 3) < 100);
    assert(output.getCost(5, 7) < 100);

    // Invalid safety margin must be rejected.
    ugv::CostMap invalid_output(11, 11);

    assert(!ugv::inflateBlockedCells(
        input,
        invalid_output,
        footprint,
        -0.10,
        100));

    // Dimension mismatch must be rejected.
    ugv::CostMap wrong_size(10, 10);

    assert(!ugv::inflateBlockedCells(
        input,
        wrong_size,
        footprint,
        0.0,
        100));

    // Invalid input resolution must be rejected.
    ugv::CostMap invalid_resolution(11, 11);

    invalid_resolution.setTimestamp(
        1'000'000'000ULL);

    invalid_resolution.setOrigin(
        -0.50,
        -0.50);

    assert(!ugv::inflateBlockedCells(
        invalid_resolution,
        output,
        footprint,
        0.0,
        100));

    // Invalid robot geometry must be rejected.
    ugv::RobotGeometry invalid_geometry;
    invalid_geometry.width_m = -1.0;

    ugv::RobotFootprint invalid_footprint(
        invalid_geometry);

    assert(!ugv::inflateBlockedCells(
        input,
        output,
        invalid_footprint,
        0.0,
        100));

    return 0;
}