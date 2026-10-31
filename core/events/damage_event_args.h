// Damage and heal events and their modifiers (core/src/base/skill.ts DamageOrHealEventArg, the ModifyDamage*
// and ModifyHeal* mixins, ZeroHealthEventArg). One modifier object passes through every modify stage, so it has
// all stages' methods, as the JS mixin does.
#pragma once

#include <optional>
#include <vector>

#include "core/events/damage_info.h"
#include "core/events/event_arg.h"

namespace gitcg {

// How a damage was dealt (core/src/mutator.ts DamageOption); heals have none.
struct DamageOption {
  std::shared_ptr<const SkillInfo> via;
  std::vector<Reaction> enabledLunarReactions;
  int callerWho = 0;
  int targetWho = 0;
  bool targetIsActive = false;
};

class DamageOrHealEventArg : public EventArg {
 public:
  // `option` empty for heals: then the source's side is looked up in `state`.
  DamageOrHealEventArg(GameStatePointer state, DamageInfo info, std::optional<DamageOption> option);

  // The damage as modified so far (subclasses apply their modifiers).
  virtual DamageInfo damageInfo() const { return info_; }
  bool isDamageTypeDamage() const { return !isDamageTypeHeal(); }
  bool isDamageTypeHeal() const { return info_.type == damage::Heal; }
  AnyState source() const { return damageInfo().source; }
  CharacterPointer target() const { return damageInfo().target; }
  DamageType type() const { return damageInfo().type; }
  double value() const { return damageInfo().value; }
  std::shared_ptr<const SkillInfo> via() const { return damageInfo().via; }
  Reaction getReaction() const;  // reaction::None when none or a heal
  bool isReactionRelatedTo(DamageType element) const;
  DamageType isSwirl() const;  // -1 when not a swirl
  bool viaSkillType(SkillType skillType) const { return via()->definition->skillType == skillType; }
  bool viaChargedAttack() const { return via()->charged; }
  bool viaPlungingAttack() const { return via()->plunging; }

  std::optional<DamageOption> option;
  int sourceWho = 0;
  int targetWho = 0;
  std::vector<Reaction> enabledLunarReactions;

 protected:
  DamageInfo info_;
};

class ModifyHealEventArg : public DamageOrHealEventArg {
 public:
  using DamageOrHealEventArg::DamageOrHealEventArg;
  DamageInfo damageInfo() const override;
  DamageInfo healInfo() const { return damageInfo(); }
  double expectedValue() const { return damageInfo().expectedValue; }
  bool cancelled() const { return cancelled_; }
  bool modifiable() const;
  void cancel() { cancelled_ = true; }
  void decreaseHeal(double value);

 private:
  double increased_ = 0;
  double decreased_ = 0;
  bool cancelled_ = false;
};

class ModifyDamageEventArg : public DamageOrHealEventArg {
 public:
  using DamageOrHealEventArg::DamageOrHealEventArg;
  DamageInfo damageInfo() const override;
  void changeDamageType(DamageType type) { newDamageType_ = type; }
  Reaction increaseDamageByReaction();
  void increaseDamage(double value) { increased_ += value; }
  void multiplyDamage(double multiplier) { multiplied_ = multiplied_.value_or(0) + multiplier; }
  void divideDamage(double divider) { divider_ *= divider; }
  void decreaseDamage(double value) { decreased_ += value; }

 private:
  std::optional<DamageType> newDamageType_;
  double increased_ = 0;
  std::optional<double> multiplied_;
  double divider_ = 1;
  double decreased_ = 0;
};

// Raised when a damage would defeat its target: a skill may make the target immune.
class ZeroHealthEventArg : public ModifyDamageEventArg {
 public:
  using ModifyDamageEventArg::ModifyDamageEventArg;
  DamageInfo damageInfo() const override;
  void markImmune();

  std::shared_ptr<const SkillInfo> immuneSkill;  // set once immune (JS _immuneInfo)
};

}  // namespace gitcg
