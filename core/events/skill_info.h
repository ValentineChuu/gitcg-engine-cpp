// A skill being used: who calls it and how (core/src/base/skill.ts SkillInfo), and the argument a skill gets.
#pragma once

#include <memory>
#include <vector>

#include "core/definitions/skill_definition.h"
#include "core/state/any_state.h"

namespace gitcg {

enum class SkillEnvironment { Normal, Precalculate, Preview };
enum class FinalizeMode { Simple, Standard };

struct SkillInfo {
  AnyState caller;
  const SkillDefinition* definition = nullptr;
  std::shared_ptr<const SkillInfo> requestBy;  // the skill that requested this one (prepared skills, talents)
  bool charged = false;
  bool plunging = false;
  bool prepared = false;
  SkillEnvironment environment = SkillEnvironment::Normal;
  FinalizeMode finalizeMode = FinalizeMode::Standard;
};

// Anything handed to a skill's action or filter: an event argument, or an initiative skill's targets.
class SkillArgument {
 public:
  virtual ~SkillArgument() = default;
};

struct InitiativeSkillArgument : SkillArgument {
  std::vector<AnyState> targets;

  InitiativeSkillArgument() = default;
  explicit InitiativeSkillArgument(std::vector<AnyState> chosen) : targets(std::move(chosen)) {}
};

// A normal, elemental or burst skill used as an action (optionally a technique too).
bool isCharacterInitiativeSkill(const SkillInfo& skill, bool allowTechnique = false);

}  // namespace gitcg
