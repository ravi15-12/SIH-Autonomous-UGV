#pragma once

#include <cmath>

namespace ugv {

struct CameraPose {
    // Camera position relative to the UGV base frame, metres.
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    // Roll, pitch and yaw, radians.
    double roll = 0.0;
    double pitch = 0.0;
    double yaw = 0.0;

    bool isFinite() const noexcept {
        return std::isfinite(x) &&
               std::isfinite(y) &&
               std::isfinite(z) &&
               std::isfinite(roll) &&
               std::isfinite(pitch) &&
               std::isfinite(yaw);
    }

    bool isValid() const noexcept {
        return isFinite() && z > 0.0;
    }
};

}  // namespace ugv
