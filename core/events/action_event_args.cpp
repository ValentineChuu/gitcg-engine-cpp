#include "core/events/action_event_args.h"

#include <algorithm>
#include <stdexcept>

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"
#include "core/rules/costs.h"
#include "core/rules/entity_lookup.h"

namespace gitcg {

int ActionEventArg::currentDiceCostSize() const {
  if (isUseSkill() && action_->skill->definition->initiativeSkillConfig) {
    return action_->skill->definition->initiativeSkillConfig->diceCostSize;
  }
  if (isPlayCard()) return diceCostSizeOfCard(onTimeState, *action_->skill->caller.entity());
  return 0;
}

bool ActionEventArg::isSkillOrTalentOf(const CharacterState& character, std::optional<SkillType> skillType) const {
  static const Name TALENT = intern("talent");
  if (isUseCharacterSkill()) {
    const SkillDefinition* used = action_->skill->definition;
    const auto& skills = character.definition->skills;
    bool own = std::any_of(skills.begin(), skills.end(), [&](const SkillDefinition* skill) { return skill->id == used->id; });
    return own && (!skillType || used->skillType == *skillType);
  }
  if (isPlayCard()) {
    if (!action_->skill->caller.entity()->definition->hasTag(TALENT)) return false;
    return std::any_of(action_->targets.begin(), action_->targets.end(), [&](const AnyState& target) { return target.id() == character.id; });
  }
  return false;
}

bool ActionEventArg::hasCardTag(Name tag) const {
  return isPlayCard() && action_->skill->caller.entity()->definition->hasTag(tag);
}

ModifyActionEventArg::ModifyActionEventArg(GameStatePointer state, std::shared_ptr<const ActionInfo> actionInfo)
    : ActionEventArg(std::move(state), std::move(actionInfo)), cost_(action_->cost), fast_(action_->fast) {}

ActionInfo ModifyActionEventArg::action() const {
  ActionInfo info = *action_;
  info.cost = cost();
  info.fast = fast_;
  return info;
}

DiceRequirement ModifyActionEventArg::cost() const {
  // JS normalizes the stored map in place on every read, and later modifications see that.
  return normalizeCost(cost_);
}

int ModifyActionEventArg::costSize() const {
  return gitcg::costSize(cost());
}

int ModifyActionEventArg::diceCostSize() const {
  return gitcg::diceCostSize(cost());
}

void ModifyActionEventArg::deductVoidCost(int count) {
  gitcg::deductCost(cost_, {dice::Void}, count);
}

void ModifyActionEventArg::addCost(DiceType type, int count) {
  if (type == dice::Omni) {
    appendCost(cost_, costPriority(), count);
  } else {
    cost_.set(type, cost_.countOf(type) + count);
  }
}

void ModifyActionEventArg::deductCost(DiceType type, int count) {
  gitcg::deductCost(cost_, {type, dice::Void}, count);
}

void ModifyActionEventArg::deductOmniCost(int count) {
  gitcg::deductCost(cost_, costPriority(), count);
}

CharacterPointer UseSkillEventArg::techniqueCaller() const {
  if (skill->definition->skillType != SkillType::Technique) throw std::runtime_error("techniqueCaller only available on technique");
  if (callerArea.type != AreaType::Characters) throw std::logic_error("Technique callerArea not in character zone");
  return getEntityById(*onTimeState, callerArea.characterId).character();
}

int PlayCardEventArg::playCost() const {
  return diceCostSize(playCardInfo->cost);
}

int PlayCardEventArg::originalPlayCost() const {
  return diceCostSize(playCardInfo->originalCost);
}

bool PlayCardEventArg::hasCardTag(Name tag) const {
  return card()->definition->hasTag(tag);
}

}  // namespace gitcg
