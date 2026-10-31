// Dice costs of skills and cards (core/src/utils.ts): normalizing, deducting, appending, and a hand card's cost
// after its attachments.
#pragma once

#include <optional>
#include <vector>

#include "core/definitions/entity_definition.h"
#include "core/definitions/skill_definition.h"
#include "core/state/game_state.h"
#include "core/values/dice_requirement.h"

namespace gitcg {

// Drops zero entries; an empty cost becomes {Aligned: 0}.
DiceRequirement& normalizeCost(DiceRequirement& cost);
int costSize(const DiceRequirement& cost);
int diceCostSize(const DiceRequirement& cost);  // without Energy and Legend

// Takes `count` off the first types of `availableTypes` in priority order.
void deductCost(DiceRequirement& cost, const std::vector<DiceType>& availableTypes, int count);
// Adds `count` to the first type of `availableTypes` already in the cost, else to Aligned.
void appendCost(DiceRequirement& cost, const std::vector<DiceType>& availableTypes, int count);

// The color priority used when adding or removing "any" dice: Aligned, the elements, Void.
const std::vector<DiceType>& costPriority();

const SkillDefinition* playSkillOfCard(const EntityDefinition& card);
const DiceRequirement& costOfCard(const EntityDefinition& card);  // {Aligned: 0} without a play skill
int originalDiceCostSizeOfCard(const EntityDefinition& card);

struct ModifiedHandCard {
  DiceRequirement requiredCost;
  bool shouldFast = false;
  bool disableTuning = false;
  bool willBeEffectless = false;
  std::optional<DiceType> tunedToType;  // nullopt: the active character's element
};

ModifiedHandCard applyAttachmentModifications(const GameStatePointer& state, const EntityState& card);
int diceCostSizeOfCard(const GameStatePointer& state, const EntityState& card);

}  // namespace gitcg
