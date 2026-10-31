// A player's action and its cost details (core/src/base/skill.ts ActionInfo): use a skill, play a card, switch,
// elemental tuning or declare end.
#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "core/events/skill_info.h"
#include "core/json/json_value.h"
#include "core/state/any_state.h"
#include "core/values/dice_requirement.h"
#include "core/values/game_enums.h"

namespace gitcg {

enum class ActionType { UseSkill, PlayCard, SwitchActive, ElementalTuning, DeclareEnd };

struct ActionInfo {
  ActionType type = ActionType::DeclareEnd;
  int who = 0;
  // UseSkill, PlayCard (the card is skill->caller).
  std::shared_ptr<const SkillInfo> skill;
  std::vector<AnyState> targets;
  std::optional<int> mainDamageTargetId;
  bool willBeEffectless = false;  // PlayCard
  // SwitchActive.
  CharacterPointer from;
  CharacterPointer to;
  bool fromReaction = false;
  // ElementalTuning.
  EntityPointer card;
  DiceType result = dice::Void;
  bool allowTuningAnyDice = false;
  // Details.
  DiceRequirement originalCost;
  DiceRequirement cost;
  bool originalFast = false;
  bool fast = false;
  int validity = 0;
  std::vector<DiceType> autoSelectedDice;
  JsonValue preview = JsonValue::array();
};

}  // namespace gitcg
