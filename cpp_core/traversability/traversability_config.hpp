#pragma once

#include <cstdint>

namespace ugv {

struct TraversabilityConfig {
    // Semantic costs
    float traversable_cost = 5.0f;
    float non_traversable_cost = 65.0f;
    float obstacle_cost = 100.0f;

    // Perception contributions
    float confidence_weight = 20.0f;
    float roughness_weight = 25.0f;
    float depth_edge_weight = 20.0f;
    float proximity_weight = 15.0f;

    // Safety thresholds
    std::uint8_t safe_threshold = 30;
    std::uint8_t caution_threshold = 65;
    std::uint8_t blocked_threshold = 100;

    // Minimum confidence required for trusted perception
    float minimum_confidence = 0.40f;

    // Maximum permitted perception-frame age.
// Units: nanoseconds.
// Provisional until validated against the complete sensor/control pipeline.
std::uint64_t max_perception_age_ns = 100'000'000ULL;
// Physical CostMap resolution.

// Units: metres per cell.

// Must be validated against the actual camera projection / mapping pipeline.

double cost_map_resolution_m = 0.05;
// CostMap origin in the local UGV frame, metres.
// This is the map reference point, not yet the camera-ground projection.
double cost_map_origin_x_m = 0.0;
double cost_map_origin_y_m = 0.0;
};

}  // namespace ugv
