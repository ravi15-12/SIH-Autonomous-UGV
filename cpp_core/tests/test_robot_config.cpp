#include "../common/robot_config.hpp"

#include <cassert>
#include <cmath>
#include <limits>

int main() {
    // Default configuration should be valid.
    {
        ugv::RobotConfig config;

        assert(config.isValid());
        assert(config.additional_safety_margin_m == 0.0);
    }

    // Valid custom safety margin.
    {
        ugv::RobotConfig config;

        config.additional_safety_margin_m = 0.10;

        assert(config.isValid());
    }

    // Negative safety margin must be rejected.
    {
        ugv::RobotConfig config;

        config.additional_safety_margin_m = -0.01;

        assert(!config.isValid());
    }

    // NaN safety margin must be rejected.
    {
        ugv::RobotConfig config;

        config.additional_safety_margin_m =
            std::numeric_limits<double>::quiet_NaN();

        assert(!config.isValid());
    }

    // Infinite safety margin must be rejected.
    {
        ugv::RobotConfig config;

        config.additional_safety_margin_m =
            std::numeric_limits<double>::infinity();

        assert(!config.isValid());
    }

    // Invalid robot geometry must invalidate the
    // complete robot configuration.
    {
        ugv::RobotConfig config;

        config.geometry.length_m = -1.0;

        assert(!config.isValid());
    }

    return 0;
}