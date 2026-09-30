#pragma once

#include "robot_config.hpp"

namespace ugv {

/*
 * RobotConfigAdapter provides the boundary between external
 * configuration systems and the C++ navigation core.
 *
 * The navigation core receives an already validated RobotConfig.
 *
 * YAML parsing itself must remain outside this header.
 */
class RobotConfigAdapter {
public:

    static bool validate(
        const RobotConfig& config
    ) noexcept {
        return config.isValid();
    }
};

}  // namespace ugv
