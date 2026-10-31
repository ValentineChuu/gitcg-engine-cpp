// Paying dice costs (@gi-tcg/utils dice.ts): the automatic choice offered with every action, and the check of
// a chosen payment. Neither looks at energy or Legend.
#pragma once

#include <set>
#include <vector>

#include "core/values/dice_requirement.h"
#include "core/values/game_enums.h"

namespace gitcg {

// Empty when the dice can't pay.
std::vector<DiceType> chooseDiceValue(const DiceRequirement& required, const std::vector<DiceType>& dice,
                                      const std::set<DiceType>& usefulDice, const std::set<DiceType>& targetDice,
                                      const std::set<DiceType>& disallowedDice);

bool checkDice(const DiceRequirement& required, const std::vector<DiceType>& chosen);

}  // namespace gitcg
