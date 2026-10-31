// Elemental reactions (core/src/base/reaction.ts): what an element does to an aura.
#pragma once

#include <vector>

#include "core/values/game_enums.h"

namespace gitcg {

struct ReactionOutcome {
  Aura newAura;
  Reaction reaction;  // reaction::None when nothing reacts
};

// `enabledLunarReactions` turns Electro-Charged, Bloom and Hydro Crystallize into their lunar forms.
ReactionOutcome getReaction(DamageType type, Aura targetAura, const std::vector<Reaction>& enabledLunarReactions);

// The two elements a reaction involves (REACTION_RELATIVES).
bool reactionInvolves(Reaction reaction, DamageType element);

// The element swirled, or -1 when the damage doesn't swirl.
DamageType swirledElement(DamageType type, Aura targetAura);

}  // namespace gitcg
