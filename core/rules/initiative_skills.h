// Skills a player can use as actions, and their targets (core/src/utils.ts initiativeSkillsOfPlayer,
// isSkillDisabled, isChargedPlunging, getPlayCardTargetCandidates).
#pragma once

#include <vector>

#include "core/events/skill_info.h"
#include "core/rules/entity_lookup.h"
#include "core/state/game_state.h"

namespace gitcg {

// The active character's equipment/status skills first (technique etc.), then its own initiative skills.
std::vector<CallerAndSkill> initiativeSkillsOfPlayer(const PlayerState& player, bool includeHidden = false);

bool isSkillDisabled(const CharacterState& character);

struct ChargedPlunging {
  bool charged = false;
  bool plunging = false;
};
ChargedPlunging chargedPlunging(const SkillDefinition& skill, const PlayerState& player);

// Targets for playing a card: a full support zone offers each support to replace.
std::vector<InitiativeSkillArgument> playCardTargetCandidates(const GameStatePointer& state, int who, const EntityPointer& card,
                                                              const SkillInfo& skill);

}  // namespace gitcg
