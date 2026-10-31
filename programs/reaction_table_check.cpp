// Prints the reaction table as JSON lines, for comparison with core/src/base/reaction.ts (tests/compare_reactions.mjs).

#include <cstdio>

#include "core/rules/reaction_table.h"

using namespace gitcg;

int main() {
  const Aura auras[] = {aura::None, aura::Cryo, aura::Hydro, aura::Pyro, aura::Electro, aura::Dendro, aura::CryoDendro};
  const std::vector<Reaction> lunarSets[] = {{}, {reaction::LunarElectroCharged, reaction::LunarBloom, reaction::LunarCrystallizeHydro}};
  for (const auto& lunar : lunarSets) {
    for (Aura targetAura : auras) {
      for (DamageType type = damage::Physical; type <= damage::Piercing; ++type) {
        ReactionOutcome outcome = getReaction(type, targetAura, lunar);
        std::printf("%zu %d %d %d %d\n", lunar.size(), targetAura, type, outcome.newAura, outcome.reaction);
      }
    }
  }
  return 0;
}
