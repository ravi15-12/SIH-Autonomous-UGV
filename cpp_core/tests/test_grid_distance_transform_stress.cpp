#include "../traversability/cost_map.hpp"
#include "../planning/grid_clearance.hpp"
#include "../planning/grid_distance_transform.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>

namespace {

void compareMap(
    const ugv::CostMap& map,
    std::uint8_t blocked_threshold
) {
    ugv::GridDistanceTransform transform;

    assert(
        transform.compute(
            map,
            blocked_threshold));

    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {

            const double reference =
                ugv::calculateCellClearance(
                    map,
                    x,
                    y,
                    blocked_threshold);

            const double transformed =
                transform.clearanceMeters(
                    map,
                    x,
                    y);

            if (std::isinf(reference)) {
                assert(std::isinf(transformed));
                continue;
            }

            assert(std::isfinite(reference));
            assert(std::isfinite(transformed));

            /*
             * Both implementations operate on the same
             * grid geometry. Allow only a very small
             * floating-point tolerance.
             */
            const double difference =
                std::abs(reference - transformed);

            assert(difference < 1e-9);
        }
    }
}

}  // namespace

int main() {
    constexpr std::uint8_t blocked_threshold = 100;

    /*
     * ---------------------------------------------------------
     * Test 1: No obstacles
     * ---------------------------------------------------------
     */
    {
        ugv::CostMap map(15, 15);

        map.setResolution(0.05);
        map.setTimestamp(1'000'000'000ULL);
        map.setOrigin(-0.375, -0.375);

        compareMap(
            map,
            blocked_threshold);
    }

    /*
     * ---------------------------------------------------------
     * Test 2: Single central obstacle
     * ---------------------------------------------------------
     */
    {
        ugv::CostMap map(15, 15);

        map.setResolution(0.05);
        map.setTimestamp(1'000'000'000ULL);
        map.setOrigin(-0.375, -0.375);

        map.setCost(7, 7, blocked_threshold);

        compareMap(
            map,
            blocked_threshold);
    }

    /*
     * ---------------------------------------------------------
     * Test 3: Multiple isolated obstacles
     * ---------------------------------------------------------
     */
    {
        ugv::CostMap map(20, 20);

        map.setResolution(0.10);
        map.setTimestamp(1'000'000'000ULL);
        map.setOrigin(-1.0, -1.0);

        map.setCost(2, 3, blocked_threshold);
        map.setCost(17, 4, blocked_threshold);
        map.setCost(5, 15, blocked_threshold);
        map.setCost(14, 16, blocked_threshold);

        compareMap(
            map,
            blocked_threshold);
    }

    /*
     * ---------------------------------------------------------
     * Test 4: Horizontal wall
     * ---------------------------------------------------------
     */
    {
        ugv::CostMap map(25, 25);

        map.setResolution(0.05);
        map.setTimestamp(1'000'000'000ULL);
        map.setOrigin(-0.625, -0.625);

        for (int x = 3; x <= 21; ++x) {
            map.setCost(x, 12, blocked_threshold);
        }

        compareMap(
            map,
            blocked_threshold);
    }

    /*
     * ---------------------------------------------------------
     * Test 5: Vertical wall
     * ---------------------------------------------------------
     */
    {
        ugv::CostMap map(25, 25);

        map.setResolution(0.05);
        map.setTimestamp(1'000'000'000ULL);
        map.setOrigin(-0.625, -0.625);

        for (int y = 3; y <= 21; ++y) {
            map.setCost(12, y, blocked_threshold);
        }

        compareMap(
            map,
            blocked_threshold);
    }

    /*
     * ---------------------------------------------------------
     * Test 6: Diagonal obstacle line
     * ---------------------------------------------------------
     */
    {
        ugv::CostMap map(20, 20);

        map.setResolution(0.10);
        map.setTimestamp(1'000'000'000ULL);
        map.setOrigin(-1.0, -1.0);

        for (int i = 3; i < 17; ++i) {
            map.setCost(i, i, blocked_threshold);
        }

        compareMap(
            map,
            blocked_threshold);
    }

    /*
     * ---------------------------------------------------------
     * Test 7: Filled obstacle block
     * ---------------------------------------------------------
     */
    {
        ugv::CostMap map(20, 20);

        map.setResolution(0.05);
        map.setTimestamp(1'000'000'000ULL);
        map.setOrigin(-0.50, -0.50);

        for (int y = 7; y <= 12; ++y) {
            for (int x = 7; x <= 12; ++x) {
                map.setCost(x, y, blocked_threshold);
            }
        }

        compareMap(
            map,
            blocked_threshold);
    }

    /*
     * ---------------------------------------------------------
     * Test 8: Border obstacles
     * ---------------------------------------------------------
     */
    {
        ugv::CostMap map(15, 15);

        map.setResolution(0.10);
        map.setTimestamp(1'000'000'000ULL);
        map.setOrigin(0.0, 0.0);

        for (int x = 0; x < map.width(); ++x) {
            map.setCost(x, 0, blocked_threshold);
            map.setCost(x, map.height() - 1, blocked_threshold);
        }

        for (int y = 0; y < map.height(); ++y) {
            map.setCost(0, y, blocked_threshold);
            map.setCost(map.width() - 1, y, blocked_threshold);
        }

        compareMap(
            map,
            blocked_threshold);
    }

    /*
     * ---------------------------------------------------------
     * Test 9: Random obstacle maps
     *
     * Randomized testing is important because it explores
     * obstacle arrangements that we did not manually design.
     * ---------------------------------------------------------
     */
    {
        constexpr int width = 20;
        constexpr int height = 20;
        constexpr int number_of_maps = 100;

        std::mt19937 generator(42);

        std::bernoulli_distribution obstacle_distribution(0.20);

        for (int map_index = 0;
             map_index < number_of_maps;
             ++map_index) {

            ugv::CostMap map(width, height);

            map.setResolution(0.05);
            map.setTimestamp(
                1'000'000'000ULL +
                static_cast<std::uint64_t>(
                    map_index));

            map.setOrigin(-0.50, -0.50);

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {

                    if (obstacle_distribution(generator)) {
                        map.setCost(
                            x,
                            y,
                            blocked_threshold);
                    }
                }
            }

            compareMap(
                map,
                blocked_threshold);
        }
    }

    /*
     * ---------------------------------------------------------
     * Test 10: Different obstacle densities
     * ---------------------------------------------------------
     */
    {
        constexpr int width = 18;
        constexpr int height = 18;

        const double densities[] = {
            0.05,
            0.10,
            0.25,
            0.50,
            0.75
        };

        for (double density : densities) {

            ugv::CostMap map(width, height);

            map.setResolution(0.10);
            map.setTimestamp(1'000'000'000ULL);
            map.setOrigin(-0.90, -0.90);

            std::mt19937 generator(
                static_cast<std::uint32_t>(
                    density * 1000.0));

            std::bernoulli_distribution distribution(
                density);

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {

                    if (distribution(generator)) {
                        map.setCost(
                            x,
                            y,
                            blocked_threshold);
                    }
                }
            }

            compareMap(
                map,
                blocked_threshold);
        }
    }

    /*
     * ---------------------------------------------------------
     * Test 11: Non-default blocked threshold
     * ---------------------------------------------------------
     */
    {
        constexpr std::uint8_t threshold = 80;

        ugv::CostMap map(12, 12);

        map.setResolution(0.05);
        map.setTimestamp(1'000'000'000ULL);
        map.setOrigin(0.0, 0.0);

        map.setCost(3, 3, 80);
        map.setCost(8, 8, 100);
        map.setCost(5, 9, 79);

        compareMap(
            map,
            threshold);
    }

    std::cout
        << "GridDistanceTransform stress tests: PASS\n";

    return 0;
}