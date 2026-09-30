#pragma once

#include <cstdint>
#include <vector>

namespace ugv {

enum class SemanticClass : std::uint8_t {
    Unknown = 0,
    Traversable = 1,
    NonTraversable = 2,
    Obstacle = 3
};

struct PerceptionFrame {
    int width = 0;
    int height = 0;

    // Monotonic timestamp assigned when the frame is captured/received.
    // Units: nanoseconds.
    std::uint64_t timestamp_ns = 0;

    // One value per pixel, row-major:
    // index = y * width + x
    std::vector<SemanticClass> segmentation;
    std::vector<float> confidence;
    std::vector<float> depth;
    std::vector<float> roughness;

    bool isValid() const noexcept {
        if (width <= 0 || height <= 0) {
            return false;
        }

        if (timestamp_ns == 0) {
            return false;
        }

        const std::size_t expected =
            static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height);

        return segmentation.size() == expected &&
               confidence.size() == expected &&
               depth.size() == expected &&
               roughness.size() == expected;
    }
};

}  // namespace ugv
