#include "../traversability/cost_map.hpp"
#include "../planning/grid_clearance.hpp"
#include "../planning/grid_distance_transform.hpp"

#include <cassert>
#include <cmath>
#include <limits>

int main() {
    ugv::CostMap map(5, 5);

    map.setResolution(0.10);
    map.setTimestamp(1'000'000'000ULL);
    map.setOrigin(0.0, 0.0);

    // ------------------------------------------------------------
    // Reference implementation: no blocked cells.
    // ------------------------------------------------------------

    const double reference_clear_distance =
        ugv::calculateCellClearance(
            map,
            2,
            2,
            100);

    assert(std::isinf(reference_clear_distance));

    // ------------------------------------------------------------
    // Distance transform: no blocked cells.
    // ------------------------------------------------------------

    ugv::GridDistanceTransform transform;

    assert(
        transform.compute(
            map,
            100));

    const double transform_clear_distance =
        transform.clearanceMeters(
            map,
            2,
            2);

    assert(std::isinf(transform_clear_distance));

    // ------------------------------------------------------------
    // One-cell horizontal distance.
    // ------------------------------------------------------------

    map.setCost(3, 2, 100);

    const double reference_one_cell =
        ugv::calculateCellClearance(
            map,
            2,
            2,
            100);

    assert(std::isfinite(reference_one_cell));
    assert(
        std::abs(
            reference_one_cell - 0.10) < 1e-9);

    assert(
        transform.compute(
            map,
            100));

    const double transform_one_cell =
        transform.clearanceMeters(
            map,
            2,
            2);

    assert(std::isfinite(transform_one_cell));
    assert(
        std::abs(
            transform_one_cell - 0.10) < 1e-9);

    // ------------------------------------------------------------
    // Diagonal distance.
    // ------------------------------------------------------------

    map.reset();
    map.setCost(3, 3, 100);

    const double reference_diagonal =
        ugv::calculateCellClearance(
            map,
            2,
            2,
            100);

    assert(std::isfinite(reference_diagonal));

    const double expected_diagonal =
        std::sqrt(2.0) * 0.10;

    assert(
        std::abs(
            reference_diagonal -
            expected_diagonal) < 1e-9);

    assert(
        transform.compute(
            map,
            100));

    const double transform_diagonal =
        transform.clearanceMeters(
            map,
            2,
            2);

    assert(std::isfinite(transform_diagonal));

    assert(
        std::abs(
            transform_diagonal -
            expected_diagonal) < 1e-9);

    // ------------------------------------------------------------
    // Nearest obstacle must be selected.
    // ------------------------------------------------------------

    map.reset();

    map.setCost(0, 0, 100);
    map.setCost(4, 2, 100);

    const double reference_nearest =
        ugv::calculateCellClearance(
            map,
            2,
            2,
            100);

    assert(std::isfinite(reference_nearest));
    assert(
        std::abs(
            reference_nearest - 0.20) < 1e-9);

    assert(
        transform.compute(
            map,
            100));

    const double transform_nearest =
        transform.clearanceMeters(
            map,
            2,
            2);

    assert(std::isfinite(transform_nearest));
    assert(
        std::abs(
            transform_nearest - 0.20) < 1e-9);

    // ------------------------------------------------------------
    // Invalid coordinates.
    // ------------------------------------------------------------

    assert(
        ugv::calculateCellClearance(
            map,
            -1,
            2,
            100) == 0.0);

    assert(
        transform.clearanceMeters(
            map,
            -1,
            2) == 0.0);

    assert(
        ugv::calculateCellClearance(
            map,
            5,
            2,
            100) == 0.0);

    assert(
        transform.clearanceMeters(
            map,
            5,
            2) == 0.0);

    // ------------------------------------------------------------
    // Invalid map resolution.
    // ------------------------------------------------------------

    ugv::CostMap invalid_map(5, 5);

    assert(
        !transform.compute(
            invalid_map,
            100));

    return 0;
}