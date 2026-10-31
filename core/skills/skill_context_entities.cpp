// SkillContext: creating, moving, equipping, attaching and disposing entities; variables; definition changes.

#include <algorithm>
#include <cmath>

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"
#include "core/definitions/game_data.h"
#include "core/mutations/state_mutations.h"
#include "core/rules/entity_lookup.h"
#include "core/skills/skill_context.h"
#include "core/values/engine_errors.h"

namespace gitcg {

namespace {

const Name& nameUsage() { static const Name name = intern("usage"); return name; }
const Name& nameLayer() { static const Name name = intern("layer"); return name; }

// Variables that raise modifyChangeVariable / onChangeVariable (runtime/skill.ts VARIABLE_NAME_CAN_EMIT_EVENTS).
bool canEmitVariableEvents(Name name) {
  static const Name NAMES[] = {intern("usage"), intern("nightsoul"), intern("effect")};
  return std::find(std::begin(NAMES), std::end(NAMES), name) != std::end(NAMES);
}

const VariableConfig* varConfigOf(const AnyState& state, Name name) {
  return state.isCharacter() ? state.character()->definition->findVarConfig(name) : state.entity()->definition->findVarConfig(name);
}

}  // namespace

std::optional<EntityHandle> SkillContext::createEntity(EntityType type, int definitionId, std::optional<EntityArea> area,
                                                       const CreateEntityOptions& options) {
  const EntityDefinition* definition = state().data->entities.find(definitionId);
  if (!definition) throw GameDataError("Unknown entity definition id " + std::to_string(definitionId));
  if (!area) {
    switch (type) {
      case EntityType::CombatStatus: area = EntityArea{AreaType::CombatStatuses, selfWho(), 0, 0}; break;
      case EntityType::Summon: area = EntityArea{AreaType::Summons, selfWho(), 0, 0}; break;
      case EntityType::Support: area = EntityArea{AreaType::Supports, selfWho(), 0, 0}; break;
      default: throw GameDataError("Creating this entity type requires an explicit area");
    }
  }
  InsertEntityResult result = mutator_.insertEntityOnStage(definition, nullptr, *area, options);
  callAndEmit(result.events);
  if (!result.newState) return std::nullopt;
  return EntityHandle(this, result.newState->id);
}

std::optional<EntityHandle> SkillContext::moveEntity(const EntityHandle& entity, const EntityArea& area, MoveReason reason) {
  EntityPointer latest = entity.latest();
  InsertEntityResult result = mutator_.insertEntityOnStage(latest->definition, latest, area, {}, reason);
  callAndEmit(result.events);
  if (!result.newState) return std::nullopt;
  return EntityHandle(this, result.newState->id);
}

std::optional<EntityHandle> SkillContext::summon(int definitionId, bool opponent, const CreateEntityOptions& options) {
  if (!opponent) return createEntity(EntityType::Summon, definitionId, std::nullopt, options);
  return createEntity(EntityType::Summon, definitionId, EntityArea{AreaType::Summons, 1 - selfWho(), 0, 0}, options);
}

void SkillContext::characterStatus(int definitionId, const CreateEntityOptions& options) {
  characterStatus(definitionId, {self()}, options);
}

void SkillContext::characterStatus(int definitionId, const std::vector<AnyHandle>& targets, const CreateEntityOptions& options) {
  for (const CharacterHandle& character : charactersOf(targets)) createEntity(EntityType::Status, definitionId, character.area(), options);
}

std::optional<EntityHandle> SkillContext::combatStatus(int definitionId, bool opponent, const CreateEntityOptions& options) {
  if (!opponent) return createEntity(EntityType::CombatStatus, definitionId, std::nullopt, options);
  return createEntity(EntityType::CombatStatus, definitionId, EntityArea{AreaType::CombatStatuses, 1 - selfWho(), 0, 0}, options);
}

namespace {

// Equipping replaces an equipment with a shared slot tag on the same character.
void disposeSameSlot(SkillContext& context, const CharacterHandle& character, const EntityDefinition& incoming) {
  static const Name SLOT_TAGS[] = {intern("artifact"), intern("weapon"), intern("technique")};
  for (Name tag : SLOT_TAGS) {
    if (!incoming.hasTag(tag)) continue;
    for (const EntityHandle& entity : character.entities()) {
      const EntityDefinition& definition = entity.definition();
      if (definition.type == EntityType::Equipment && definition.hasTag(tag)) {
        context.dispose(std::vector<AnyHandle>{AnyHandle(&context, entity.id(), false)}, RemoveReason::Overflow, true);
        break;
      }
    }
  }
}

}  // namespace

void SkillContext::equip(int definitionId, const std::vector<AnyHandle>& targets, const CreateEntityOptions& options) {
  const EntityDefinition* definition = state().data->entities.find(definitionId);
  if (!definition) throw GameDataError("Unknown equipment definition id " + std::to_string(definitionId));
  for (const CharacterHandle& character : charactersOf(targets)) {
    disposeSameSlot(*this, character, *definition);
    createEntity(EntityType::Equipment, definitionId, character.area(), options);
  }
}

void SkillContext::equip(const EntityHandle& card, const std::vector<AnyHandle>& targets) {
  const EntityDefinition& definition = card.definition();
  for (const CharacterHandle& character : charactersOf(targets)) {
    disposeSameSlot(*this, character, definition);
    moveEntity(card, character.area(), MoveReason::Equip);
  }
}

void SkillContext::unequip(const EntityHandle& equipment) {
  EntityArea area = equipment.area();
  EntityPointer latest = equipment.latest();
  if (area.type != AreaType::Characters) throw GameDataError("Can only unequip from characters");
  resetVariables(mutator_.mutableState(), latest->id, false);
  gitcg::moveEntity(mutator_.mutableState(), latest->id, EntityArea{AreaType::Hands, area.who, 0, latest->id}, std::nullopt, MoveReason::Unequip,
                    latest.get());  // the core passes the state from before the reset
}

void SkillContext::attach(int attachmentId, const EntityHandle& target, const CreateEntityOptions& options) {
  const EntityDefinition* definition = state().data->attachments.find(attachmentId);
  if (!definition) throw GameDataError("Unknown attachment definition id " + std::to_string(attachmentId));
  InsertEntityResult result = mutator_.createAttachment(target.latest(), definition, options);
  callAndEmit(result.events);
}

void SkillContext::attachCostChange(const EntityHandle& target, double value, bool increase) {
  const int COST_INCREASE = 201;
  const int COST_REDUCTION = 202;
  int consumed = increase ? COST_REDUCTION : COST_INCREASE;
  int incoming = increase ? COST_INCREASE : COST_REDUCTION;
  // $.def(consumed).on($.id(target.id)): the first such attachment on the card.
  std::optional<EntityHandle> existed;
  for (const EntityPointer& attachment : target.attachments()) {
    if (attachment->definition->id == consumed) {
      existed = EntityHandle(this, attachment->id);
      break;
    }
  }
  if (existed) {
    double existedLayer = existed->getVariable(nameLayer());
    if (existedLayer > value) {
      setVariable(nameLayer(), existedLayer - value, AnyHandle(this, existed->id(), false));
    } else {
      removeEntity(mutator_.mutableState(), existed->id(), existed->area().who);
    }
    value -= existedLayer;
  }
  if (value > 0) {
    CreateEntityOptions options;
    options.overrideVariables = {{nameLayer(), value}};
    attach(incoming, target, options);
  }
}

void SkillContext::attachCostIncrease(const std::vector<AnyHandle>& targets, double value) {
  for (const AnyHandle& target : targets) attachCostChange(target.asEntity(), value, true);
}

void SkillContext::attachCostReduction(const std::vector<AnyHandle>& targets, double value) {
  for (const AnyHandle& target : targets) attachCostChange(target.asEntity(), value, false);
}

void SkillContext::dispose(std::optional<std::vector<AnyHandle>> targets, RemoveReason reason, bool direct) {
  std::vector<AnyHandle> resolved = targets ? *targets : std::vector<AnyHandle>{self()};
  for (const AnyHandle& handle : resolved) {
    if (handle.isCharacter()) throw GameDataError("Character caller cannot be disposed");
    EntityHandle entity = handle.asEntity();
    EntityPointer target = entity.latest();
    const EntityDefinition& definition = *target->definition;
    double usage = target->variables.valueOr(nameUsage(), 0);
    if (!direct && definition.type != EntityType::Attachment && usage > 0 && definition.disposeWhenUsageIsZero) {
      setVariable(nameUsage(), 0, handle);
      target = entity.latest();
    }
    EntityArea area = entity.area();
    emitEvent(EventName::OnDispose, std::make_shared<DisposeEventArg>(rawState(), target, reason, area, sharedSkillInfo()));
    removeEntity(mutator_.mutableState(), target->id, area.who, reason);
  }
}

double SkillContext::getVariable(Name name) {
  return getVariable(name, self());
}

double SkillContext::getVariable(Name name, const AnyHandle& target) {
  return target.latest().variables().valueOr(name, 0);
}

void SkillContext::setVariable(Name name, double value) {
  setVariable(name, value, self());
}

void SkillContext::setVariable(Name name, double value, const AnyHandle& target) {
  AnyState latest = target.latest();
  const VariableConfig* config = varConfigOf(latest, name);
  if (!config) throw GameDataError("Variable " + nameText(name) + " has no config");
  value = clampVariable(value, *config);
  std::optional<double> oldValue = latest.variables().find(name);
  if (!oldValue) {
    // JS reads undefined: the difference is NaN and `value >= undefined` is false, so the direction is "decrease".
    setVariableImpl(target, {name, std::nan(""), std::nan(""), value, false, false});
    return;
  }
  setVariableImpl(target, {name, *oldValue, value - *oldValue, value, value >= *oldValue, false});
}

void SkillContext::addVariable(Name name, double value) {
  addVariable(name, value, self());
}

void SkillContext::addVariable(Name name, double value, const AnyHandle& target) {
  setVariable(name, value + target.latest().variables().valueOr(name, 0), target);
}

void SkillContext::consumeUsage(double count) {
  if (skillInfo_.caller.isCharacter()) throw GameDataError("Cannot consume usage of character");
  consumeUsage(count, self());
}

void SkillContext::consumeUsage(double count, const AnyHandle& target) {
  AnyState latest = target.latest();
  if (!varConfigOf(latest, nameUsage())) return;
  double current = getVariable(nameUsage(), target);
  if (current <= 0) return;
  addVariable(nameUsage(), -std::min(count, current), target);
  if (latest.entity()->definition->disposeWhenUsageIsZero && getVariable(nameUsage(), target) <= 0) {
    dispose(std::vector<AnyHandle>{target}, RemoveReason::Other, true);
  }
}

void SkillContext::consumeUsagePerRound(double count) {
  if (!skillInfo_.definition->usagePerRoundVariableName) throw GameDataError("This skill has no usagePerRound");
  Name name = *skillInfo_.definition->usagePerRoundVariableName;
  double current = getVariable(name, self());
  if (current > 0) addVariable(name, -std::min(count, current), self());
}

void SkillContext::setVariableImpl(const AnyHandle& target, VariableChangeInfo info) {
  AnyState latest = target.latest();
  if (canEmitVariableEvents(info.varName)) {
    BeforeVariableEventArg modifyEvent(rawState(), latest, info);
    EventsAndDefeat inner = mutator_.handleInlineEvent(skillInfo_, EventName::ModifyChangeVariable, modifyEvent);
    callAndEmit(inner.events);
    info = modifyEvent.info;
    if (info.cancelled) return;
  }
  modifyEntityVariable(mutator_.mutableState(), latest.id(), info.varName, info.newValue, info.increase ? 1 : 2);
  emitEvent(EventName::OnChangeVariable, std::make_shared<VariableEventArg>(rawState(), target.latest(), info));
}

void SkillContext::transformDefinition(const std::vector<AnyHandle>& targets, int newDefinitionId) {
  for (const AnyHandle& handle : targets) {
    AnyState target = handle.latest();
    DefinitionReference newDefinition;
    if (target.isCharacter()) {
      newDefinition.character = state().data->characters.find(newDefinitionId);
      if (!newDefinition.character) throw GameDataError("Unknown definition id " + std::to_string(newDefinitionId));
      transformCharacterDefinition(mutator_.mutableState(), target.id(), newDefinition.character);
    } else {
      bool attachment = target.entity()->definition->type == EntityType::Attachment;
      newDefinition.entity = attachment ? state().data->attachments.find(newDefinitionId) : state().data->entities.find(newDefinitionId);
      if (!newDefinition.entity) throw GameDataError("Unknown definition id " + std::to_string(newDefinitionId));
      if (newDefinition.entity->type != target.entity()->definition->type) {
        throw GameInternalError("Cannot transform definition between different types");
      }
      transformEntityDefinition(mutator_.mutableState(), target.id(), newDefinition.entity);
    }
    emitEvent(EventName::OnTransformDefinition, std::make_shared<TransformDefinitionEventArg>(rawState(), target, newDefinition));
  }
}

}  // namespace gitcg
