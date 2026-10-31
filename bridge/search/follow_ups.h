#pragma once
#include "core/events/action_info.h"
#include "core/state/game_state.h"
namespace gitcg {
JsonValue followUps(const GameStatePointer& start, int who, const std::vector<ActionInfo>& actions, int index, int budget = 150);
}
