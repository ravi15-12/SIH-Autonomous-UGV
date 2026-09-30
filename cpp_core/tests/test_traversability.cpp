#include "../common/perception_frame.hpp"
#include "../traversability/cost_map.hpp"
#include "../traversability/traversability_config.hpp"
#include "../traversability/traversability_engine.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

int main() {

    constexpr int WIDTH = 4;
    constexpr int HEIGHT = 2;
    constexpr std::size_t PIXELS = WIDTH * HEIGHT;

    ugv::TraversabilityConfig config;
    ugv::TraversabilityEngine engine(config);

    ugv::PerceptionFrame frame;
    frame.width = WIDTH;
    frame.height = HEIGHT;
    frame.timestamp_ns = 1;

    frame.segmentation.resize(PIXELS);
    frame.confidence.resize(PIXELS, 1.0f);
    frame.depth.resize(PIXELS, 0.0f);
    frame.roughness.resize(PIXELS, 0.0f);

    // --------------------------------------------------------
    // 1. Basic semantic classes
    // --------------------------------------------------------

    frame.segmentation[0] =
        ugv::SemanticClass::Traversable;

    frame.segmentation[1] =
        ugv::SemanticClass::NonTraversable;

    frame.segmentation[2] =
        ugv::SemanticClass::Obstacle;

    frame.segmentation[3] =
        ugv::SemanticClass::Unknown;

    ugv::CostMap map(WIDTH, HEIGHT);

    assert(engine.compute(frame,frame.timestamp_ns, map));

    assert(map.getCost(0, 0) < 30);
    assert(map.getCost(1, 0) >= 65);
    assert(map.getCost(2, 0) == 100);
    assert(map.getCost(3, 0) >= 65);

    // --------------------------------------------------------
    // 2. Low confidence must be conservative
    // --------------------------------------------------------

    frame.segmentation[0] =
        ugv::SemanticClass::Traversable;

    frame.confidence[0] = 0.20f;

    assert(engine.compute(frame,frame.timestamp_ns, map));

    assert(map.getCost(0, 0) >= 65);

    // Restore confidence.
    frame.confidence[0] = 1.0f;

    // --------------------------------------------------------
    // 3. Roughness must increase cost
    // --------------------------------------------------------

    frame.segmentation[0] =
        ugv::SemanticClass::Traversable;

    frame.roughness[0] = 0.0f;

    assert(engine.compute(frame,frame.timestamp_ns, map));

    const auto low_roughness_cost =
        map.getCost(0, 0);

    frame.roughness[0] = 1.0f;

    assert(engine.compute(frame,frame.timestamp_ns, map));

    const auto high_roughness_cost =
        map.getCost(0, 0);

    assert(high_roughness_cost > low_roughness_cost);

    // --------------------------------------------------------
    // 4. Costs must remain within [0, 100]
    // --------------------------------------------------------

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {

            const auto cost =
                map.getCost(x, y);

            assert(cost <= 100);
        }
    }

    // --------------------------------------------------------
    // 5. Invalid perception must be rejected
    // --------------------------------------------------------

    ugv::PerceptionFrame invalid_frame;

    assert(!engine.compute(invalid_frame,invalid_frame.timestamp_ns, map));

    // --------------------------------------------------------
    // 6. Dimension mismatch must be rejected
    // --------------------------------------------------------

    ugv::CostMap wrong_size_map(2, 2);

    assert(!engine.compute(frame,frame.timestamp_ns, wrong_size_map));

    // --------------------------------------------------------
    // 7. Invalid confidence must be rejected/conservative
    // --------------------------------------------------------

    frame.confidence[0] = 1.5f;

    assert(engine.compute(frame,frame.timestamp_ns, map));

    assert(map.getCost(0, 0) == 100);

    std::cout << "Traversability tests: PASS\n";
    {
    ugv::PerceptionFrame frame;
    frame.width = 2;
    frame.height = 2;
    frame.timestamp_ns = 5'000'000'000ULL;

    frame.segmentation.assign(
        4, ugv::SemanticClass::Traversable);
    frame.confidence.assign(4, 1.0f);
    frame.depth.assign(4, 0.0f);
    frame.roughness.assign(4, 0.0f);

    ugv::CostMap map(2, 2);
    ugv::TraversabilityEngine engine(config);

    assert(engine.compute(
        frame,
        5'050'000'000ULL,
        map));

    assert(map.timestamp() == frame.timestamp_ns);
    assert(map.resolution() == config.cost_map_resolution_m);
    assert(map.originX() == config.cost_map_origin_x_m);
    assert(map.originY() == config.cost_map_origin_y_m);
}

    return 0;
}
