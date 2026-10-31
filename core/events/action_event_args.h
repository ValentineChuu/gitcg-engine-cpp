// Action events (core/src/base/skill.ts ActionEventArg, the ModifyAction* mixin, UseSkillEventArg,
// PlayCardEventArg, SwitchActiveEventArg).
#pragma once

#include <initializer_list>
#include <memory>
#include <optional>

#include "core/events/action_info.h"
#include "core/events/event_arg.h"
#include "core/state/entity_area.h"

namespace gitcg {

class ActionEventArg : public PlayerEventArg {
 public:
  ActionEventArg(GameStatePointer state, std::shared_ptr<const ActionInfo> actionInfo)
      : PlayerEventArg(std::move(state), actionInfo->who), action_(std::move(actionInfo)) {}

  virtual ActionInfo action() const { return *action_; }
  int currentDiceCostSize() const;
  bool isSwitchActive() const { return action_->type == ActionType::SwitchActive; }
  bool isPlayCard() const { return action_->type == ActionType::PlayCard; }
  bool isUseSkill() const { return action_->type == ActionType::UseSkill; }
  bool isDeclareEnd() const { return action_->type == ActionType::DeclareEnd; }
  bool isUseCharacterSkill() const { return isUseSkill() && isCharacterInitiativeSkill(*action_->skill); }
  bool isUseTechnique() const { return isSkillType(SkillType::Technique); }
  bool isSkillOrTalentOf(const CharacterState& character, std::optional<SkillType> skillType = std::nullopt) const;
  bool isSkillType(SkillType skillType) const { return isUseSkill() && action_->skill->definition->skillType == skillType; }
  bool isChargedAttack() const { return isUseSkill() && action_->skill->charged; }
  bool isPlungingAttack() const { return isUseSkill() && action_->skill->plunging; }
  bool hasCardTag(Name tag) const;
  bool hasOneOfCardTag(std::initializer_list<Name> tags) const {
    for (Name tag : tags) {
      if (hasCardTag(tag)) return true;
    }
    return false;
  }

 protected:
  std::shared_ptr<const ActionInfo> action_;
};

// One object passes through modifyAction0..4, so it has every stage's methods.
class ModifyActionEventArg : public ActionEventArg {
 public:
  ModifyActionEventArg(GameStatePointer state, std::shared_ptr<const ActionInfo> actionInfo);

  ActionInfo action() const override;
  DiceRequirement cost() const;  // normalizes the stored cost in place first, as the JS getter does
  int costSize() const;
  int diceCostSize() const;
  bool isFast() const { return fast_; }
  bool canDeductVoidCost() const { return cost().has(dice::Void); }
  bool canDeductCostOfType(DiceType type) const { return cost().has(type) || cost().has(dice::Void); }
  bool canDeductCost() const { return diceCostSize() > 0; }
  // modifyAction0
  void deductVoidCost(int count);
  void addCost(DiceType type, int count);
  // modifyAction1
  void deductCost(DiceType type, int count);
  // modifyAction2
  void deductOmniCost(int count);
  // modifyAction3
  void deductAllCost() { cost_ = DiceRequirement(); }
  // modifyAction4
  void setFastAction() { fast_ = true; }

 private:
  mutable DiceRequirement cost_;
  bool fast_;
};

struct SwitchActiveInfo {
  int who = 0;
  CharacterPointer from;  // null at the start of the game
  std::shared_ptr<const SkillInfo> via;
  CharacterPointer to;
  bool fromReaction = false;
  std::optional<bool> fast;  // nullopt when not an action
};

class SwitchActiveEventArg : public EventArg {
 public:
  SwitchActiveEventArg(GameStatePointer state, SwitchActiveInfo info) : EventArg(std::move(state)), switchInfo(std::move(info)) {}
  SwitchActiveInfo switchInfo;
};

class UseSkillEventArg : public PlayerEventArg {
 public:
  UseSkillEventArg(GameStatePointer state, EntityArea area, std::shared_ptr<const SkillInfo> skillInfo)
      : PlayerEventArg(std::move(state), area.who), callerArea(area), skill(std::move(skillInfo)) {}

  const AnyState& skillCaller() const { return skill->caller; }
  CharacterPointer techniqueCaller() const;
  bool isSkillType(SkillType skillType) const { return skill->definition->skillType == skillType; }
  bool isChargedAttack() const { return skill->charged; }
  bool isPlungingAttack() const { return skill->plunging; }

  EntityArea callerArea;
  std::shared_ptr<const SkillInfo> skill;
};

class PlayCardEventArg : public PlayerEventArg {
 public:
  PlayCardEventArg(GameStatePointer state, std::shared_ptr<const ActionInfo> info)
      : PlayerEventArg(std::move(state), info->who), playCardInfo(std::move(info)) {}

  int playCost() const;
  int originalPlayCost() const;
  const EntityPointer& card() const { return playCardInfo->skill->caller.entity(); }
  bool hasCardTag(Name tag) const;
  bool hasOneOfCardTag(std::initializer_list<Name> tags) const {
    for (Name tag : tags) {
      if (hasCardTag(tag)) return true;
    }
    return false;
  }

  std::shared_ptr<const ActionInfo> playCardInfo;
};

}  // namespace gitcg
