#include "core/events/damage_event_args.h"

#include <algorithm>
#include <cmath>

#include "core/rules/entity_lookup.h"
#include "core/rules/reaction_table.h"

namespace gitcg {

DamageOrHealEventArg::DamageOrHealEventArg(GameStatePointer state, DamageInfo info, std::optional<DamageOption> damageOption)
    : EventArg(std::move(state)), option(std::move(damageOption)), info_(std::move(info)) {
  sourceWho = option ? option->callerWho : getEntityArea(*onTimeState, info_.source.id()).who;
  targetWho = getEntityArea(*onTimeState, info_.target->id).who;
  if (option) enabledLunarReactions = option->enabledLunarReactions;
}

Reaction DamageOrHealEventArg::getReaction() const {
  if (!isDamageTypeDamage()) return reaction::None;
  DamageInfo info = damageInfo();
  return gitcg::getReaction(info.type, info.targetAura, enabledLunarReactions).reaction;
}

bool DamageOrHealEventArg::isReactionRelatedTo(DamageType element) const {
  if (!isDamageTypeDamage()) return false;
  DamageInfo info = damageInfo();
  Reaction plain = gitcg::getReaction(info.type, info.targetAura, {}).reaction;
  return plain != reaction::None && reactionInvolves(plain, element);
}

DamageType DamageOrHealEventArg::isSwirl() const {
  if (!isDamageTypeDamage()) return -1;
  DamageInfo info = damageInfo();
  return swirledElement(info.type, info.targetAura);
}

DamageInfo ModifyHealEventArg::damageInfo() const {
  DamageInfo info = info_;
  static const Name MAX_HEALTH = intern("maxHealth");
  static const Name HEALTH = intern("health");
  double expected = std::max(0.0, std::ceil(info.expectedValue + increased_ - decreased_));
  double targetLoss = info.target->variables.valueOr(MAX_HEALTH, 0) - info.target->variables.valueOr(HEALTH, 0);
  info.expectedValue = expected;
  info.value = std::min(expected, targetLoss);
  info.cancelled = cancelled_;
  return info;
}

bool ModifyHealEventArg::modifiable() const {
  HealKind kind = info_.healKind;
  return kind != HealKind::ImmuneDefeated && kind != HealKind::Revive;
}

void ModifyHealEventArg::decreaseHeal(double value) {
  if (cancelled_) return;
  decreased_ += value;
}

DamageInfo ModifyDamageEventArg::damageInfo() const {
  static const Name HEALTH = intern("health");
  DamageInfo info = info_;
  double targetHealth = info.target->variables.valueOr(HEALTH, 0);
  info.type = newDamageType_.value_or(info.type);
  double value = info.value + increased_;
  double multiplier = multiplied_.value_or(1) / divider_;
  value = std::ceil(value * multiplier);
  value = std::max(0.0, value - decreased_);
  info.value = value;
  info.causeDefeated = value >= targetHealth;
  return info;
}

Reaction ModifyDamageEventArg::increaseDamageByReaction() {
  Reaction reactionType = getReaction();
  switch (reactionType) {
    case reaction::Melt:
    case reaction::Vaporize:
    case reaction::Overloaded: increased_ += 2; break;
    case reaction::Superconduct:
    case reaction::ElectroCharged:
    case reaction::Frozen:
    case reaction::CrystallizeCryo:
    case reaction::CrystallizeHydro:
    case reaction::CrystallizePyro:
    case reaction::CrystallizeElectro:
    case reaction::Burning:
    case reaction::Bloom:
    case reaction::Quicken:
    case reaction::LunarBloom:
    case reaction::LunarCrystallizeHydro: increased_ += 1; break;
    default: break;
  }
  return reactionType;
}

DamageInfo ZeroHealthEventArg::damageInfo() const {
  DamageInfo info = ModifyDamageEventArg::damageInfo();
  info.causeDefeated = info.causeDefeated && !immuneSkill;
  return info;
}

void ZeroHealthEventArg::markImmune() {
  immuneSkill = std::make_shared<SkillInfo>(*currentSkillInfo);
}

}  // namespace gitcg
