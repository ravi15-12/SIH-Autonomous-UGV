#pragma once

#include "../cpp_core/transport/metric_map_reader.hpp"
#include "../cpp_core/traversability/cost_map.hpp"

namespace ugv {

struct MetricMapGeometry {
    double resolution_m = 0.05;
    double forward_min_m = 0.0;
    double left_min_m = -4.0;
};

class MetricMapCostMapAdapter {
public:
    static bool convert(
        const MetricMapFrame& frame,
        const MetricMapGeometry& geometry,
        CostMap& output
    );
};

}  // namespace ugv