// A skill: something a character, entity or extension does when used as an action or when an event fires
// (core/src/base/skill.ts SkillDefinition). `action` and `filter` are built by the definition VM.
#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "core/definitions/event_name.h"
#include "core/state/game_state.h"
#include "core/values/dice_requirement.h"
#include "core/values/names.h"

namespace gitcg {

struct SkillInfo;
class SkillArgument;
struct SkillOutcome;
struct InitiativeSkillArgument;

enum class SkillType { None, Normal, Elemental, Burst, Technique, PlayCard };
enum class OwnerType { Character, EventCard, Status, CombatStatus, Equipment, Support, Summon, Attachment, Extension };

using SkillAction = std::function<SkillOutcome(const GameStatePointer& state, const SkillInfo& skill, SkillArgument& argument)>;
using SkillFilter = std::function<bool(const GameStatePointer& state, const SkillInfo& skill, SkillArgument& argument)>;
using TargetGetter = std::function<std::vector<InitiativeSkillArgument>(const GameStatePointer& state, const SkillInfo& skill)>;

struct InitiativeSkillConfig {
  DiceRequirement requiredCost;
  int costSize = 0;
  int diceCostSize = 0;
  TargetGetter getTarget;
  bool gainEnergy = true;
  bool shouldFast = false;
  bool alwaysCharged = false;
  bool alwaysPlunging = false;
  bool hidden = false;
  bool omitEvents = false;
};

struct SkillDefinition {
  double id = 0;  // triggered skills have fractional sub-ids (mainId + 0.01 n)
  OwnerType ownerType = OwnerType::Character;
  SkillType skillType = SkillType::None;
  EventName triggerOn = EventName::Initiative;
  SkillAction action;
  SkillFilter filter;
  std::optional<Name> usagePerRoundVariableName;
  std::optional<InitiativeSkillConfig> initiativeSkillConfig;
};

}  // namespace gitcg
