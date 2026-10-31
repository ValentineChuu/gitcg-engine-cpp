// A new game's state from two decks (core/src/game.ts Game.createInitialState). Decks come pre-shuffled from the
// bridge (noShuffle): the core's own shuffle uses Math.random and is never used.
#pragma once

#include <cstdint>
#include <vector>

#include "core/definitions/game_data.h"
#include "core/state/game_state.h"

namespace gitcg {

struct DeckList {
  std::vector<int> characters;
  std::vector<int> cards;
};

GameStatePointer createInitialState(const GameData& data, const DeckList& first, const DeckList& second, int64_t randomSeed);

}  // namespace gitcg
