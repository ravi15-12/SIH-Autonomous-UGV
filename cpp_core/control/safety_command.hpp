#pragma once

#include "motion_controller.hpp"

namespace ugv {

enum class SafetyState {
    SafeToMove,
    Stop
};

struct SafetyCommand {
    PlannerCommand command{};
    SafetyState state = SafetyState::Stop;
};

class SafetyCommandGate {
public:
    static SafetyCommand apply(
        const PlannerCommand& command,
        bool map_valid,
        bool path_valid
    ) noexcept;
};

}  // namespace ugv
