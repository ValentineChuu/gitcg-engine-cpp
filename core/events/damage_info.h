// One damage or heal (core/src/base/skill.ts DamageInfo and HealInfo; a heal has type Heal and the heal fields).
#pragma once

#include <memory>
#include <string>

#include "core/events/skill_info.h"
#include "core/state/any_state.h"
#include "core/values/game_enums.h"

namespace gitcg {

enum class HealKind { Common, ImmuneDefeated, Revive, Distribution, IncreaseMaxHealth };

struct DamageInfo {
  DamageType type = damage::Physical;
  double value = 0;
  AnyState source;
  std::shared_ptr<const SkillInfo> via;
  CharacterPointer target;
  Aura targetAura = aura::None;           // damage only
  bool isSkillMainDamage = false;         // damage only
  bool causeDefeated = false;
  Reaction fromReaction = reaction::None;  // damage only; None = JS null
  std::string log;
  // Heal only.
  int healId = 0;
  bool cancelled = false;
  double expectedValue = 0;
  HealKind healKind = HealKind::Common;

  bool isHeal() const { return type == damage::Heal; }
};

}  // namespace gitcg
