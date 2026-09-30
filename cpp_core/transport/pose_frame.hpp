#pragma once

#include <cstdint>

namespace ugv {

struct PoseFrame {
    std::uint64_t sequence = 0;
    std::uint64_t timestamp_ns = 0;

    double x_m = 0.0;
    double y_m = 0.0;
    double yaw_rad = 0.0;

    bool isValid() const noexcept;
};

}  // namespace ugv