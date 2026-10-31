// The core's own effect of each elemental reaction (core/src/reaction.ts): Overloaded switches, Superconduct and
// Electro-Charged pierce, Frozen freezes, Swirl spreads, Crystallize shields, and so on.
#pragma once

#include "core/definitions/skill_definition.h"
#include "core/values/game_enums.h"

namespace gitcg {

// Null for reactions without a core effect (Melt, Vaporize).
const SkillDefinition* reactionSkill(Reaction reaction);

}  // namespace gitcg
