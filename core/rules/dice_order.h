// Orders of dice (core/src/utils.ts sortDice and computeConvertDice, @gi-tcg/utils diceIndex).
#pragma once

#include <vector>

#include "core/state/player_state.h"
#include "core/values/game_enums.h"

namespace gitcg {

// Official dice order is Cryo Hydro Pyro Electro Geo Dendro Anemo: Anemo sorts after Omni.
inline int diceIndex(DiceType type) {
  return type == dice::Anemo ? dice::Omni + 1 : type;
}

// Display order: omni first, then the player's useful elements, then more numerous, then dice index. Stable.
std::vector<DiceType> sortDice(const PlayerState& player, const std::vector<DiceType>& dice);

// The player's dice after converting `count` (or all, when count < 0) of them to `target`: omni and target dice
// are kept; the rest convert useless first, then fewer, then by dice index.
std::vector<DiceType> computeConvertDice(const PlayerState& player, DiceType target, int count);

}  // namespace gitcg
