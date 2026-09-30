#include "../common/robot_config_factory.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    const ugv::RobotConfig config =
        ugv::RobotConfigFactory::create(
            0.40,  // length
            0.30,  // width
            0.30,  // wheel base
            0.30,  // track width
            0.05,  // front overhang
            0.05,  // rear overhang
            0.00   // safety margin
        );

    assert(config.isValid());

    assert(std::abs(config.geometry.length_m - 0.40) < 1e-9);
    assert(std::abs(config.geometry.width_m - 0.30) < 1e-9);
    assert(std::abs(config.geometry.wheel_base_m - 0.30) < 1e-9);
    assert(std::abs(config.geometry.track_width_m - 0.30) < 1e-9);
    assert(std::abs(config.geometry.front_overhang_m - 0.05) < 1e-9);
    assert(std::abs(config.geometry.rear_overhang_m - 0.05) < 1e-9);
    assert(
        std::abs(config.additional_safety_margin_m - 0.00) < 1e-9
    );

    std::cout << "RobotConfigFactory: PASS\n";
    return 0;
}