#pragma once

#include "robot_config.hpp"

namespace ugv {

class RobotConfigFactory {
public:
    static RobotConfig create(
        double length_m,
        double width_m,
        double wheel_base_m,
        double track_width_m,
        double front_overhang_m,
        double rear_overhang_m,
        double additional_safety_margin_m) noexcept
    {
        RobotConfig config;

        config.geometry.length_m = length_m;
        config.geometry.width_m = width_m;
        config.geometry.wheel_base_m = wheel_base_m;
        config.geometry.track_width_m = track_width_m;
        config.geometry.front_overhang_m = front_overhang_m;
        config.geometry.rear_overhang_m = rear_overhang_m;

        config.additional_safety_margin_m =
            additional_safety_margin_m;

        return config;
    }
};

}  // namespace ugv