#pragma once

#include <cstdint>

namespace ugv {

class FrameAgePolicy {
public:
    explicit FrameAgePolicy(std::uint64_t max_age_ns)
        : max_age_ns_(max_age_ns) {}

    bool isFresh(
        std::uint64_t frame_timestamp_ns,
        std::uint64_t now_timestamp_ns
    ) const noexcept {

        if (frame_timestamp_ns == 0 ||
            now_timestamp_ns == 0) {
            return false;
        }

        if (now_timestamp_ns < frame_timestamp_ns) {
            return false;
        }

        return (now_timestamp_ns - frame_timestamp_ns) <= max_age_ns_;
    }

private:
    std::uint64_t max_age_ns_;
};

}  // namespace ugv
