#pragma once
#include "core/state/game_state.h"
namespace gitcg {
JsonValue opponentCardsFor(const GameStatePointer& state, int who, double seed, const JsonValue& view,
                          const std::vector<int>& played);
}
