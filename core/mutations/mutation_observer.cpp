#include "core/mutations/mutation_observer.h"
#include <cmath>
#include "core/events/action_event_args.h"
#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"

namespace gitcg {
thread_local MutationObserver* mutationObserver = nullptr;
ObservedSkill::ObservedSkill()
    : previous_(mutationObserver), observer_([this](JsonValue value) { records_.push_back(std::move(value)); }) {
  if (previous_) mutationObserver = &observer_;
}
ObservedSkill::~ObservedSkill() { if (!finished_) mutationObserver = previous_; }
void ObservedSkill::finish(bool changed) {
  mutationObserver = previous_;
  finished_ = true;
  if (previous_ && changed) for (auto& record : records_) (*previous_)(std::move(record));
}

void observeMutation(const char* kind, JsonValue value) {
  if (!mutationObserver) return;
  JsonValue mutation = JsonValue::object();
  mutation["$case"] = kind;
  mutation["value"] = std::move(value);
  (*mutationObserver)(std::move(mutation));
}

int exposedArea(AreaType type) {
  switch (type) {
    case AreaType::Characters: return 1;
    case AreaType::CombatStatuses: return 2;
    case AreaType::Summons: return 3;
    case AreaType::Supports: return 4;
    case AreaType::Hands: return 5;
    case AreaType::Pile: return 6;
    default: return 0;
  }
}
int exposedRemoveReason(RemoveReason reason) {
  switch (reason) {
    case RemoveReason::EventCardPlayed: return 1;
    case RemoveReason::ElementalTuning: return 2;
    case RemoveReason::Overflow: return 3;
    case RemoveReason::Discarded: return 4;
    case RemoveReason::EventCardPlayNoEffect: return 5;
    case RemoveReason::EventCardDrawn: return 6;
    case RemoveReason::EquipOverridden: return 7;
    case RemoveReason::CreateSupportOverridden: return 8;
    case RemoveReason::TargetOfSupportPlayed: return 9;
    default: return 0;
  }
}
int exposedMoveReason(MoveReason reason) {
  switch (reason) {
    case MoveReason::Switch: return 1;
    case MoveReason::Draw: return 2;
    case MoveReason::Undraw: return 3;
    case MoveReason::Steal: return 4;
    case MoveReason::Swap: return 5;
    case MoveReason::Equip: return 6;
    case MoveReason::CreateSupport: return 7;
    case MoveReason::Unequip: return 8;
    case MoveReason::Unsupport: return 9;
    default: return 0;
  }
}
void observeDamage(const DamageInfo& info, Aura newAura, double oldHealth, double newHealth, Reaction reaction) {
  if (!mutationObserver) return;
  JsonValue value = JsonValue::object();
  value["damageType"] = info.type;
  value["sourceId"] = info.source.id();
  value["sourceDefinitionId"] = (info.source.isCharacter() ? info.source.character()->definition->id : info.source.entity()->definition->id);
  value["value"] = info.value;
  value["targetId"] = info.target->id;
  value["targetDefinitionId"] = info.target->definition->id;
  value["isSkillMainDamage"] = info.isHeal() ? false : info.isSkillMainDamage;
  value["reactionType"] = reaction;
  value["causeDefeated"] = info.isHeal() ? false : info.causeDefeated;
  value["oldAura"] = info.isHeal() ? info.target->variables.valueOr(intern("aura"), 0) : info.targetAura;
  value["newAura"] = newAura;
  value["oldHealth"] = oldHealth;
  value["newHealth"] = newHealth;
  int healKind = 0;
  if (info.isHeal()) {
    switch (info.healKind) {
      case HealKind::Common: healKind = 1; break;
      case HealKind::ImmuneDefeated: healKind = 2; break;
      case HealKind::Revive: healKind = 3; break;
      case HealKind::IncreaseMaxHealth: healKind = 4; break;
      case HealKind::Distribution: healKind = 5; break;
    }
  }
  value["healKind"] = healKind;
  observeMutation("damage", std::move(value));
}
void observeAura(int element, const CharacterPointer& target, Aura newAura, Reaction reaction) {
  if (!mutationObserver) return;
  JsonValue value = JsonValue::object();
  value["elementType"] = element;
  value["targetId"] = target->id;
  value["targetDefinitionId"] = target->definition->id;
  value["reactionType"] = reaction;
  value["oldAura"] = target->variables.valueOr(intern("aura"), 0);
  value["newAura"] = newAura;
  observeMutation("applyAura", std::move(value));
}
void observeSwitch(const SwitchActiveInfo& info) {
  if (!mutationObserver) return;
  JsonValue value = JsonValue::object();
  value["who"] = info.who;
  value["characterId"] = info.to->id;
  value["characterDefinitionId"] = info.to->definition->id;
  if (info.fromReaction) value["viaSkillDefinitionId"] = reaction::Overloaded;
  else if (info.via) value["viaSkillDefinitionId"] = std::floor(info.via->definition->id);
  value["fromAction"] = info.fast ? (*info.fast ? 2 : 1) : 0;
  observeMutation("switchActive", std::move(value));
}
}  // namespace gitcg
