#include "../common/robot_config_adapter.hpp"

#include <cassert>

int main() {
    // Valid configuration must be accepted.
    {
        ugv::RobotConfig config;

        assert(
            ugv::RobotConfigAdapter::validate(config));
    }

    // Invalid geometry must be rejected.
    {
        ugv::RobotConfig config;

        config.geometry.length_m = -1.0;

        assert(
            !ugv::RobotConfigAdapter::validate(config));
    }

    // Invalid safety margin must be rejected.
    {
        ugv::RobotConfig config;

        config.additional_safety_margin_m = -0.01;

        assert(
            !ugv::RobotConfigAdapter::validate(config));
    }

    return 0;
}