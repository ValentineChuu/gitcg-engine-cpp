// Events about one entity or character (core/src/base/skill.ts EntityEventArg, EnterEventArg, DisposeEventArg,
// CharacterEventArg, ReactionEventArg, ModifyReactionEventArg, TransformDefinitionEventArg,
// HandCardInsertedEventArg, GenerateDiceEventArg, VariableEventArg, BeforeVariableEventArg, CustomEventEventArg).
#pragma once

#include <memory>
#include <string>

#include "core/events/damage_info.h"
#include "core/events/event_arg.h"
#include "core/json/json_value.h"
#include "core/state/entity_area.h"

namespace gitcg {

struct CharacterDefinition;
struct EntityDefinition;

enum class RemoveReason {
  Discarded, TargetOfSupportPlayed, EventCardPlayed, EventCardDrawn, EquipOverridden, CreateSupportOverridden,
  ElementalTuning, Overflow, EventCardPlayNoEffect, Other,
};
enum class MoveReason { Switch, Draw, Undraw, Steal, Swap, Equip, CreateSupport, Unequip, Unsupport, Other };
// Why a card entered a hand: a move reason, or "create".
enum class HandInsertReason { Switch, Draw, Undraw, Steal, Swap, Equip, CreateSupport, Unequip, Unsupport, Other, Create };

class EntityEventArg : public EventArg {
 public:
  EntityEventArg(GameStatePointer state, AnyState subject);
  AnyState entity;
  EntityArea area;
  int who;
};

class EnterEventArg : public EntityEventArg {
 public:
  EnterEventArg(GameStatePointer state, AnyState newState, EntityPointer overriddenState)
      : EntityEventArg(std::move(state), std::move(newState)), overridden(std::move(overriddenState)) {}
  EntityPointer overridden;  // null when nothing was overridden; read as a snapshot, never "latest"
};

class DisposeEventArg : public EntityEventArg {
 public:
  DisposeEventArg(GameStatePointer state, EntityPointer disposed, RemoveReason removeReason, EntityArea fromArea,
                  std::shared_ptr<const SkillInfo> viaSkill)
      : EntityEventArg(std::move(state), disposed), reason(removeReason), from(fromArea), via(std::move(viaSkill)) {}

  const EntityPointer& disposedEntity() const { return entity.entity(); }
  bool isDiscard() const { return reason == RemoveReason::Discarded; }
  bool isTuning() const { return reason == RemoveReason::ElementalTuning; }
  bool isDiscardOrTuning() const { return isDiscard() || isTuning(); }
  int diceCost() const;

  RemoveReason reason;
  EntityArea from;
  std::shared_ptr<const SkillInfo> via;
};

class CharacterEventArg : public EventArg {
 public:
  CharacterEventArg(GameStatePointer state, CharacterPointer subject);
  CharacterPointer character;
  int who;
};

struct ReactionInfo {
  Reaction type = reaction::None;
  std::shared_ptr<const SkillInfo> via;
  CharacterPointer target;
  std::shared_ptr<const DamageInfo> fromDamage;  // null for a pure element application
};

class ReactionEventArg : public CharacterEventArg {
 public:
  ReactionEventArg(GameStatePointer state, ReactionInfo info);

  int viaWho() const { return callerArea_.who; }
  const AnyState& reactionCaller() const { return reactionInfo.via->caller; }
  const CharacterPointer& target() const { return reactionInfo.target; }
  Reaction type() const { return reactionInfo.type; }
  bool viaCharacterSkill() const { return isCharacterInitiativeSkill(*reactionInfo.via); }
  bool relatedTo(DamageType element) const;

  ReactionInfo reactionInfo;

 private:
  EntityArea callerArea_;
};

class ModifyReactionEventArg : public ReactionEventArg {
 public:
  ModifyReactionEventArg(GameStatePointer state, ReactionInfo info);
  void cancelCoreEffects() { coreEffectsCancelled = true; }

  bool whereIsMine;  // where: "my" when the reaction's caller and target are on the same side
  bool coreEffectsCancelled = false;
};

// A character's or an entity's definition (transformDefinition works on both).
struct DefinitionReference {
  const CharacterDefinition* character = nullptr;
  const EntityDefinition* entity = nullptr;
  int id() const;
};

class TransformDefinitionEventArg : public EventArg {
 public:
  TransformDefinitionEventArg(GameStatePointer state, AnyState subject, DefinitionReference transformedTo);
  AnyState entity;
  int who;
  DefinitionReference oldDefinition;
  DefinitionReference newDefinition;
};

class HandCardInsertedEventArg : public PlayerEventArg {
 public:
  HandCardInsertedEventArg(GameStatePointer state, int player, EntityPointer insertedCard, HandInsertReason insertReason, bool wasOverflowed)
      : PlayerEventArg(std::move(state), player), card(std::move(insertedCard)), reason(insertReason), overflowed(wasOverflowed) {}
  EntityPointer card;
  HandInsertReason reason;
  bool overflowed;
};

class GenerateDiceEventArg : public PlayerEventArg {
 public:
  GenerateDiceEventArg(GameStatePointer state, int player, std::shared_ptr<const SkillInfo> viaSkill, DiceType generated)
      : PlayerEventArg(std::move(state), player), via(std::move(viaSkill)), dice(generated) {}
  std::shared_ptr<const SkillInfo> via;
  DiceType dice;
};

struct VariableChangeInfo {
  Name varName = 0;
  double oldValue = 0;
  double diffValue = 0;
  double newValue = 0;
  bool increase = true;  // direction "increase" or "decrease"
  bool cancelled = false;
};

class VariableEventArg : public EntityEventArg {
 public:
  VariableEventArg(GameStatePointer state, AnyState subject, VariableChangeInfo change)
      : EntityEventArg(std::move(state), std::move(subject)), info(change) {}
  VariableChangeInfo info;
};

// modifyChangeVariable: a skill may cancel the change.
class BeforeVariableEventArg : public VariableEventArg {
 public:
  using VariableEventArg::VariableEventArg;
  void cancel() { info.cancelled = true; }
};

// A custom event's identity: card data creates each once and compares by identity.
struct CustomEvent {
  std::string name;
};

class CustomEventEventArg : public EntityEventArg {
 public:
  CustomEventEventArg(GameStatePointer state, AnyState subject, const CustomEvent* event, JsonValue eventArgument)
      : EntityEventArg(std::move(state), std::move(subject)), customEvent(event), arg(std::move(eventArgument)) {}
  const CustomEvent* customEvent;
  JsonValue arg;
  // An event argument passed through as-is (`:handleCustomEventInline(X, :rawEventArg)`); it outlives the inline
  // handling, which runs inside the handler that received it.
  const SkillArgument* rawArg = nullptr;
};

}  // namespace gitcg
