// Query-visible dice cost from utils.ts; energy and legend requirements never count as dice (AGPL-3.0).
#include "query/card_cost.h"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include "core/definitions/entity_definition.h"

namespace gitcg::query {
double cardDiceCost(const GameState& state, const EntityPointer& card) {
  const InitiativeSkillConfig* play = nullptr;
  for (const auto* skill : card->definition->skills) {
    if (skill->skillType == SkillType::PlayCard) {
      if (skill->initiativeSkillConfig) play = &*skill->initiativeSkillConfig;
      break;
    }
  }
  if (!state.versionBehavior.diceCostApplyAttachments || card->attachments.empty()) return play ? play->diceCostSize : 0;
  DiceRequirement costs;
  if (play) costs = play->requiredCost;
  else costs.set(dice::Aligned, 0);
  auto snapshot = makeRef<GameState>(state);
  std::vector<AttachmentModification> modifications;
  for (const auto& attachment : card->attachments) {
    if (!attachment->definition->modifications) throw std::runtime_error("Attachment cost modifications need executable definitions");
    auto additional = attachment->definition->modifications(snapshot, attachment->id);
    modifications.insert(modifications.end(), additional.begin(), additional.end());
  }
  const DiceType priority[] = {dice::Aligned, dice::Cryo, dice::Hydro, dice::Pyro, dice::Electro,
      dice::Anemo, dice::Geo, dice::Dendro, dice::Void};
  std::optional<DiceType> changedType;
  for (const auto& modification : modifications) {
    if (modification.kind == AttachmentModification::Kind::IncreaseCardCost) {
      DiceType target = dice::Aligned;
      for (DiceType type : priority) if (costs.has(type)) { target = type; break; }
      costs.set(target, costs.countOf(target) + modification.value);
    } else if (modification.kind == AttachmentModification::Kind::DecreaseCardCost) {
      int count = modification.value;
      for (DiceType type : priority) {
        int current = costs.countOf(type);
        costs.set(type, std::max(0, current - count));
        count -= current;
        if (count <= 0) break;
      }
    } else if (modification.kind == AttachmentModification::Kind::ChangeCardCostType) changedType = modification.diceType;
  }
  auto size = [](const DiceRequirement& cost) {
    double result = 0;
    for (const auto& [type, count] : cost.entries()) if (type != dice::Energy && type != dice::Legend) result += count;
    return result;
  };
  if (changedType) {
    double diceSize = size(costs);
    int energy = costs.countOf(dice::Energy), legend = costs.countOf(dice::Legend);
    costs.clear(); costs.set(*changedType, diceSize);
    if (energy > 0) costs.set(dice::Energy, energy);
    if (legend > 0) costs.set(dice::Legend, legend);
  }
  return size(costs);
}
}  // namespace gitcg::query
