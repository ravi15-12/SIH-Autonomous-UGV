#include "traversability_engine.hpp"

#include <algorithm>
#include <cmath>

namespace ugv {

TraversabilityEngine::TraversabilityEngine(
    const TraversabilityConfig& config
)
    : config_(config),
    frame_age_policy_(config.max_perception_age_ns){}

bool TraversabilityEngine::compute(
    const PerceptionFrame& perception,
    std::uint64_t now_timestamp_ns,
    CostMap& output
) const noexcept {
    if (!std::isfinite(config_.cost_map_resolution_m) ||
    config_.cost_map_resolution_m <= 0.0) {
    return false;
}

    // Reject invalid perception data.
    if (!perception.isValid()) {
        return false;
    }
    if (!frame_age_policy_.isFresh(
        perception.timestamp_ns,
        now_timestamp_ns)) {
    return false;
}

    // Ensure output dimensions match the perception frame.
    if (output.width() != perception.width ||
        output.height() != perception.height) {
        return false;
    }
    output.setTimestamp(perception.timestamp_ns);
    output.setResolution(config_.cost_map_resolution_m);
    output.setOrigin(
    config_.cost_map_origin_x_m,
    config_.cost_map_origin_y_m
    );

    const std::size_t pixel_count =
        static_cast<std::size_t>(perception.width) *
        static_cast<std::size_t>(perception.height);

    for (std::size_t i = 0; i < pixel_count; ++i) {

        const SemanticClass semantic =
            perception.segmentation[i];

        const float confidence =
            perception.confidence[i];

        const float depth =
            perception.depth[i];

        const float roughness =
            perception.roughness[i];

        // Invalid numerical perception is treated conservatively.
        if (!std::isfinite(confidence) ||
            !std::isfinite(depth) ||
            !std::isfinite(roughness) ||
            confidence < 0.0f ||
            confidence > 1.0f) {

            const int x =
                static_cast<int>(i %
                    static_cast<std::size_t>(perception.width));

            const int y =
                static_cast<int>(i /
                    static_cast<std::size_t>(perception.width));

            output.setCost(x, y, config_.blocked_threshold);
            continue;
        }

        float cost = semanticCost(semantic);

        // Unknown or low-confidence perception should never
        // become artificially safer.
        if (semantic == SemanticClass::Unknown ||
            confidence < config_.minimum_confidence) {

            cost = std::max(
                cost,
                static_cast<float>(config_.caution_threshold)
            );
        }

        // Roughness is expected to be normalized to [0, 1].
        const float normalized_roughness =
            std::clamp(roughness, 0.0f, 1.0f);

        cost +=
            normalized_roughness *
            config_.roughness_weight;

        // Depth is relative rather than metric, so we do not
        // interpret it as physical distance here.
        //
        // For now, depth contributes only when its value is
        // explicitly normalized to [0, 1].
        const float normalized_depth =
            std::clamp(depth, 0.0f, 1.0f);

        cost +=
            normalized_depth *
            config_.depth_edge_weight;

        cost = clampCost(cost);

        const int x =
            static_cast<int>(i %
                static_cast<std::size_t>(perception.width));

        const int y =
            static_cast<int>(i /
                static_cast<std::size_t>(perception.width));

        output.setCost(
            x,
            y,
            static_cast<std::uint8_t>(std::lround(cost))
        );
    }

    return true;
}

float TraversabilityEngine::semanticCost(
    SemanticClass semantic
) const noexcept {

    switch (semantic) {

        case SemanticClass::Traversable:
            return config_.traversable_cost;

        case SemanticClass::NonTraversable:
            return config_.non_traversable_cost;

        case SemanticClass::Obstacle:
            return config_.obstacle_cost;

        case SemanticClass::Unknown:
        default:
            return static_cast<float>(
                config_.blocked_threshold
            );
    }
}

float TraversabilityEngine::clampCost(
    float cost
) const noexcept {

    if (!std::isfinite(cost)) {
        return static_cast<float>(
            config_.blocked_threshold
        );
    }

    return std::clamp(
        cost,
        0.0f,
        static_cast<float>(config_.blocked_threshold)
    );
}

}  // namespace ugv
