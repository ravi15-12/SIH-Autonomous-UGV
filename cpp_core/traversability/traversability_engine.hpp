#pragma once

#include "../common/perception_frame.hpp"
#include "cost_map.hpp"
#include "traversability_config.hpp"
#include "../common/frame_age_policy.hpp"
#include <cstdint>

namespace ugv {

class TraversabilityEngine {
public:
    explicit TraversabilityEngine(
        const TraversabilityConfig& config
    );

    bool compute(
        const PerceptionFrame& perception,
        std::uint64_t now_timestamp_ns,
        CostMap& output
    ) const noexcept;

private:
    TraversabilityConfig config_;
    FrameAgePolicy frame_age_policy_;

    float semanticCost(SemanticClass semantic) const noexcept;

    float clampCost(float cost) const noexcept;
};

}  // namespace ugv
