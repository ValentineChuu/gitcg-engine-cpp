#include "core/rules/costs.h"

#include <algorithm>

#include "core/rules/entity_lookup.h"

namespace gitcg {

namespace {

const DiceRequirement& emptyCost() {
  static const DiceRequirement cost = [] {
    DiceRequirement value;
    value.set(dice::Aligned, 0);
    return value;
  }();
  return cost;
}

}  // namespace

DiceRequirement& normalizeCost(DiceRequirement& cost) {
  std::vector<DiceType> empty;
  for (const auto& [type, count] : cost.entries()) {
    if (count == 0) empty.push_back(type);
  }
  for (DiceType type : empty) cost.erase(type);
  if (cost.size() == 0) cost.set(dice::Aligned, 0);
  return cost;
}

int costSize(const DiceRequirement& cost) {
  int total = 0;
  for (const auto& [type, count] : cost.entries()) total += count;
  return total;
}

int diceCostSize(const DiceRequirement& cost) {
  int total = 0;
  for (const auto& [type, count] : cost.entries()) {
    if (type != dice::Energy && type != dice::Legend) total += count;
  }
  return total;
}

void deductCost(DiceRequirement& cost, const std::vector<DiceType>& availableTypes, int count) {
  for (DiceType type : availableTypes) {
    int current = cost.countOf(type);
    cost.set(type, std::max(0, current - count));
    count -= current;
    if (count <= 0) return;
  }
}

void appendCost(DiceRequirement& cost, const std::vector<DiceType>& availableTypes, int count) {
  for (DiceType type : availableTypes) {
    if (auto current = cost.find(type)) {
      cost.set(type, *current + count);
      return;
    }
  }
  cost.set(dice::Aligned, count);
}

const std::vector<DiceType>& costPriority() {
  static const std::vector<DiceType> priority = {dice::Aligned, dice::Cryo, dice::Hydro, dice::Pyro, dice::Electro,
                                                 dice::Anemo,   dice::Geo,  dice::Dendro, dice::Void};
  return priority;
}

const SkillDefinition* playSkillOfCard(const EntityDefinition& card) {
  for (const SkillDefinition* skill : card.skills) {
    if (skill->skillType == SkillType::PlayCard) return skill;
  }
  return nullptr;
}

const DiceRequirement& costOfCard(const EntityDefinition& card) {
  const SkillDefinition* skill = playSkillOfCard(card);
  return skill ? skill->initiativeSkillConfig->requiredCost : emptyCost();
}

int originalDiceCostSizeOfCard(const EntityDefinition& card) {
  const SkillDefinition* skill = playSkillOfCard(card);
  return skill ? skill->initiativeSkillConfig->diceCostSize : 0;
}

ModifiedHandCard applyAttachmentModifications(const GameStatePointer& state, const EntityState& card) {
  static const Name EVENT_EFFECTLESS = intern("eventEffectless");
  int who = getEntityArea(*state, card.id).who;
  const SkillDefinition* playSkill = playSkillOfCard(*card.definition);
  ModifiedHandCard result;
  result.requiredCost = playSkill ? playSkill->initiativeSkillConfig->requiredCost : emptyCost();
  result.shouldFast = playSkill ? playSkill->initiativeSkillConfig->shouldFast : false;
  result.disableTuning = card.definition->disableTuning;
  const auto& combatStatuses = state->players[who]->combatStatuses;
  result.willBeEffectless = card.definition->type == EntityType::EventCard &&
                            std::any_of(combatStatuses.begin(), combatStatuses.end(),
                                        [](const EntityPointer& status) { return status->definition->hasTag(EVENT_EFFECTLESS); });
  std::optional<DiceType> changedCostType;
  for (const EntityPointer& attachment : card.attachments) {
    if (!attachment->definition->modifications) continue;
    for (const AttachmentModification& modification : attachment->definition->modifications(state, attachment->id)) {
      switch (modification.kind) {
        case AttachmentModification::Kind::IncreaseCardCost: appendCost(result.requiredCost, costPriority(), modification.value); break;
        case AttachmentModification::Kind::DecreaseCardCost: deductCost(result.requiredCost, costPriority(), modification.value); break;
        case AttachmentModification::Kind::ChangeCardCostType: changedCostType = modification.diceType; break;
        case AttachmentModification::Kind::ChangeCardTuningTarget: result.tunedToType = modification.diceType; break;
        case AttachmentModification::Kind::DisableCardTuning: result.disableTuning = true; break;
        case AttachmentModification::Kind::MakeEffectless: result.willBeEffectless = true; break;
      }
    }
  }
  if (changedCostType) {
    int diceSize = diceCostSize(result.requiredCost);
    int energyCost = result.requiredCost.countOf(dice::Energy);
    int legendCost = result.requiredCost.countOf(dice::Legend);
    DiceRequirement changed;
    changed.set(*changedCostType, diceSize);
    if (energyCost > 0) changed.set(dice::Energy, energyCost);
    if (legendCost > 0) changed.set(dice::Legend, legendCost);
    result.requiredCost = changed;
  }
  normalizeCost(result.requiredCost);
  return result;
}

int diceCostSizeOfCard(const GameStatePointer& state, const EntityState& card) {
  if (!state->versionBehavior.diceCostApplyAttachments || card.attachments.empty()) {
    return originalDiceCostSizeOfCard(*card.definition);
  }
  return diceCostSize(applyAttachmentModifications(state, card).requiredCost);
}

}  // namespace gitcg
