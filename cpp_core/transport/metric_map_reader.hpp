#pragma once

#include "metric_map_frame_header.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ugv {

struct MetricMapFrame {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint64_t sequence = 0;
    std::uint64_t timestamp_ns = 0;

    std::vector<std::uint8_t> cost_map;
    std::vector<std::uint8_t> observed;

    bool isValid() const noexcept {
        if (width == 0 || height == 0) {
            return false;
        }

        const std::size_t expected =
            static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height);

        return sequence > 0 &&
               timestamp_ns > 0 &&
               cost_map.size() == expected &&
               observed.size() == expected;
    }
};

class MetricMapReader {
public:
    bool decode(
        const std::uint8_t* frame,
        std::size_t frame_size,
        MetricMapFrame& output
    ) const noexcept;
};

}  // namespace ugv