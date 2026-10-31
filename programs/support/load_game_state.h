// Reconstitutes the internal-state dump, using the definitions holder's stable pointers.
#pragma once

#include "core/definitions/game_data.h"
#include "core/state/game_state.h"

namespace gitcg::testing {
GameStatePointer loadGameState(const GameData& data, const JsonValue& source);
// Event classes are not implemented in the foundation; keep their JSON alongside the loaded query state.
struct LoadedGameState { GameStatePointer state; JsonValue source; };
inline LoadedGameState loadInternalState(const GameData& data, const JsonValue& source) {
  return {loadGameState(data, source), source};
}
}  // namespace gitcg::testing
