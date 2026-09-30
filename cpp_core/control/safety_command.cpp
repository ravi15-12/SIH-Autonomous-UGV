#include "safety_command.hpp"

namespace ugv {

SafetyCommand SafetyCommandGate::apply(
    const PlannerCommand& command,
    bool map_valid,
    bool path_valid
) noexcept {

    SafetyCommand result{};

    if (!map_valid || !path_valid || !command.valid) {
        result.state = SafetyState::Stop;
        result.command = PlannerCommand{};
        return result;
    }

    result.command = command;
    result.state = SafetyState::SafeToMove;

    return result;
}

}  // namespace ugv
