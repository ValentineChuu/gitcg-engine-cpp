#pragma once
#include "core/events/action_info.h"
#include "core/state/game_state.h"
namespace gitcg {
JsonValue opponentReplies(const GameStatePointer& state, int who, const std::vector<ActionInfo>& actions, double seed,
                          const JsonValue& view, const std::vector<int>& opponentPlayed);
}
