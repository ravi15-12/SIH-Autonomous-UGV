#pragma once

#include "robot_geometry.hpp"
#include <cmath>

namespace ugv {

struct RobotConfig {
    RobotGeometry geometry{};

    double additional_safety_margin_m = 0.0;

    bool isValid() const noexcept {
        return geometry.isValid() &&
               std::isfinite(additional_safety_margin_m) &&
               additional_safety_margin_m >= 0.0;
    }
};

}  // namespace ugv