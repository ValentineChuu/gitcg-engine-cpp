// Element and other traits read from a character definition's tags (core/src/utils.ts).
#pragma once

#include <set>

#include "core/definitions/character_definition.h"
#include "core/state/player_state.h"
#include "core/values/game_enums.h"

namespace gitcg {

// The dice type of the first element tag, or Void when there is none.
DiceType elementOfCharacter(const CharacterDefinition& character);

// Elements of the player's alive characters ("useful dice").
std::set<DiceType> playerUsefulDice(const PlayerState& player);

bool isCharacterAlive(const CharacterState& character);

}  // namespace gitcg
