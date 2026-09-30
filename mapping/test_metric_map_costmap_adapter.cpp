#include "metric_map_costmap_adapter.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    constexpr std::uint32_t width = 160;
    constexpr std::uint32_t height = 160;

    ugv::MetricMapFrame frame;

    frame.width = width;
    frame.height = height;
    frame.sequence = 1;
    frame.timestamp_ns = 123456789;

    const std::size_t pixels =
        static_cast<std::size_t>(width) * height;

    frame.cost_map.resize(pixels, 100);
    frame.observed.resize(pixels, 0);

    frame.cost_map[10 * width + 20] = 25;
    frame.cost_map[30 * width + 40] = 65;

    frame.observed[10 * width + 20] = 1;
    frame.observed[30 * width + 40] = 1;

    ugv::MetricMapGeometry geometry;

    ugv::CostMap output(
        static_cast<int>(width),
        static_cast<int>(height)
    );

    assert(
        ugv::MetricMapCostMapAdapter::convert(
            frame,
            geometry,
            output
        )
    );

    assert(output.width() == 160);
    assert(output.height() == 160);

    assert(output.resolution() == 0.05);
    assert(output.originX() == 0.0);
    assert(output.originY() == -4.0);
    assert(output.timestamp() == 123456789);

    assert(output.getCost(10, 20) == 25);
    assert(output.getCost(30, 40) == 65);
    assert(output.getCost(50, 60) == 100);

    std::cout << "METRIC MAP -> COST MAP: PASS\n";

    return 0;
}