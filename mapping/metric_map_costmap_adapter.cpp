#include "metric_map_costmap_adapter.hpp"

#include <cmath>

namespace ugv {

bool MetricMapCostMapAdapter::convert(
    const MetricMapFrame& frame,
    const MetricMapGeometry& geometry,
    CostMap& output
) {
    if (!frame.isValid()) {
        return false;
    }

    if (!std::isfinite(geometry.resolution_m) ||
        geometry.resolution_m <= 0.0 ||
        !std::isfinite(geometry.forward_min_m) ||
        !std::isfinite(geometry.left_min_m)) {
        return false;
    }

    if (output.width() != static_cast<int>(frame.width) ||
        output.height() != static_cast<int>(frame.height)) {
        return false;
    }

    output.reset();

    output.setResolution(geometry.resolution_m);
    output.setTimestamp(frame.timestamp_ns);

    output.setOrigin(
        geometry.forward_min_m,
        geometry.left_min_m
    );

    for (std::uint32_t forward = 0;
         forward < frame.height;
         ++forward) {

        for (std::uint32_t left = 0;
             left < frame.width;
             ++left) {

            const std::size_t index =
                static_cast<std::size_t>(forward) *
                frame.width +
                left;

            output.setCost(
                static_cast<int>(forward),
                static_cast<int>(left),
                frame.cost_map[index]
            );
        }
    }

    return true;
}

}  // namespace ugv