// Hidden information for a look-ahead copy (reference/bridge/redeal.mjs).
#pragma once

#include "core/state/game_state.h"

namespace gitcg {

GameStatePointer redealHiddenCards(const GameStatePointer& state, int searchingPlayer, double seed,
                                  const JsonValue* opponentCards = nullptr);

}  // namespace gitcg
