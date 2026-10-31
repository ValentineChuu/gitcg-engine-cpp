#pragma once
#include "core/state/game_state.h"
namespace gitcg {
JsonValue endOfRound(const GameStatePointer& state, int who, double seed, const JsonValue* opponentCards = nullptr);
}
