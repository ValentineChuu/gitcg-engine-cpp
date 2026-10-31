// Shield, cleanup, prepare, conflict, nightsoul and card placement/target behavior against Node's VM.
#include "definitions/verification/builtin_probe.h"
#include "definitions/registry/registry.h"
#include "definitions/attributes/lambda_slots.h"
#include "core/definitions/game_data.h"
#include "core/events/action_event_args.h"
#include "core/game/initial_state.h"
#include "core/mutations/state_mutations.h"
#include "core/rules/entity_lookup.h"
#include "core/skills/skill_context.h"
#include <stdexcept>

namespace gitcg {
namespace {
GameStatePointer initial(const GameData& data) {
  DeckList deck{{9001, 9002}, {}};
  auto state = createInitialState(data, deck, deck, 1);
  changePhase(state, Phase::Action); stepRound(state);
  for (int who : {0, 1}) setActiveCharacter(state, who, state->players[who]->characters[0]->id);
  return state;
}
EntityPointer add(GameStatePointer& state, int definitionId, AreaType type = AreaType::Characters, int index = 0) {
  EntityState entity; entity.definition = state->data->entities.find(definitionId);
  for (const auto& [name, config] : entity.definition->varConfigs) entity.variables.set(name, config.initialValue);
  EntityArea area{type, 0, type == AreaType::Characters ? state->players[0]->characters[index]->id : 0};
  int id = createEntity(state, area, entity);
  return getEntityById(*state, id).entity();
}
const SkillDefinition* skillOf(EntityPointer entity, EventName name) {
  for (const auto* skill : entity->definition->skills) if (skill->triggerOn == name) return skill;
  throw std::runtime_error("Missing fixture skill");
}
SkillInfo info(EntityPointer caller, const SkillDefinition* skill) {
  SkillInfo value; value.caller = caller; value.definition = skill; value.finalizeMode = FinalizeMode::Simple;
  return value;
}
SkillOutcome action(GameStatePointer state, EntityPointer caller, const SkillDefinition* skill, SkillArgument& argument) {
  auto outcome = skill->action(state, info(caller, skill), argument);
  if (outcome.error) std::rethrow_exception(outcome.error);
  return outcome;
}
JsonValue variables(const Variables& variables) {
  auto value = JsonValue::object();
  for (const auto& [name, number] : variables.entries()) value[nameText(name)] = number;
  return value;
}
JsonValue definitionIds(const std::vector<EntityPointer>& entities) {
  auto result = JsonValue::array(); for (const auto& entity : entities) result.push(entity->definition->id); return result;
}
JsonValue targetIds(GameStatePointer state, EntityPointer entity, const SkillDefinition* skill) {
  auto result = JsonValue::array();
  for (const auto& argument : skill->initiativeSkillConfig->getTarget(state, info(entity, skill))) {
    auto targets = JsonValue::array();
    for (const auto& target : argument.targets) targets.push(target.isCharacter() ? target.character()->definition->id : target.entity()->definition->id);
    result.push(targets);
  }
  return result;
}
}  // namespace
JsonValue probeDefinitionBuiltins(std::string_view fixture) {
  DefinitionRegistry registry;
  registry.lambdaSlots()->bind(701, [](LambdaInvocation&) { return JsonValue(); });
  registry.lambdaSlots()->bind(702, [](LambdaInvocation&) { return JsonValue(); });
  registry.lambdaSlots()->bind(703, [](LambdaInvocation& invocation) { return JsonValue(invocation.extensionState->at("counts").asArray()[0].asInt() + 2.1256789); });
  registry.lambdaSlots()->bind(704, [](LambdaInvocation&) { return JsonValue(42); });
  registry.lambdaSlots()->bind(705, [](LambdaInvocation& invocation) {
    int who = static_cast<PlayerEventArg&>(*invocation.argument).who;
    auto& count = (*invocation.extensionState)["counts"].asArray().at(who);
    count = count.asInt() + 1; return JsonValue();
  });
  registry.addModule(fixture); auto data = registry.resolve();
  auto result = JsonValue::object();
  {
    auto state = initial(data); auto entity = add(state, 990001); const auto* skill = skillOf(entity, EventName::ModifyDamage3);
    DamageInfo damage; damage.type = damage::Pyro; damage.value = 3; damage.target = state->players[0]->characters[0]; damage.source = state->players[1]->characters[0];
    ModifyDamageEventArg event(state, damage, std::nullopt);
    result["shieldFilter"] = skill->filter(state, info(entity, skill), event);
    auto next = action(state, entity, skill, event).state;
    result["shield"] = JsonValue::Object{{"damage", event.value()}, {"remaining", static_cast<int>(next->players[0]->characters[0]->entities.size())}, {"removed", static_cast<int>(next->players[0]->removedEntities.size())}};
  }
  {
    auto state = initial(data); auto entity = add(state, 990002); auto* skill = skillOf(entity, EventName::OnEndPhase);
    EventArg event(state); auto used = action(state, entity, skill, event).state;
    auto after = used->players[0]->characters[0]->entities[0];
    result["usageAfter"] = after->variables.valueOr(intern("usagePerRound"), -1);
    auto* round = skillOf(entity, EventName::OnRoundEnd); EventArg end(used);
    auto next = action(used, after, round, end).state;
    result["roundReset"] = variables(next->players[0]->characters[0]->entities[0]->variables);
    EventArg final(next); auto expired = action(next, next->players[0]->characters[0]->entities[0], round, final).state;
    result["expired"] = expired->players[0]->characters[0]->entities.empty();
  }
  {
    auto state = initial(data); auto entity = add(state, 990003); auto* skill = skillOf(entity, EventName::ReplaceAction);
    PlayerEventArg event(state, 0); auto outcome = action(state, entity, skill, event);
    auto remaining = JsonValue::array();
    for (const auto& entry : outcome.state->players[0]->characters[0]->entities) remaining.push(JsonValue::Object{{"id", entry->definition->id}, {"variables", variables(entry->variables)}});
    auto events = JsonValue::array(); for (const auto& entry : outcome.emittedEvents) events.push(eventNameText(entry.name));
    result["prepare"] = JsonValue::Object{{"remaining", remaining}, {"events", events}};
  }
  {
    auto state = initial(data); add(state, 990004, AreaType::Characters, 1); auto entity = add(state, 990008); auto* skill = skillOf(entity, EventName::OnEnter);
    EnterEventArg event(state, entity, nullptr); auto next = action(state, entity, skill, event).state;
    auto entities = JsonValue::array(); for (const auto& character : next->players[0]->characters) entities.push(definitionIds(character->entities));
    result["conflict"] = entities;
  }
  {
    auto state = initial(data); auto entity = add(state, 990007); auto* skill = skillOf(entity, EventName::OnBeforeAction);
    PlayerEventArg event(state, 1);
    result["nightsoulFilter"] = skill->filter(state, info(entity, skill), event);
    auto next = action(state, entity, skill, event).state;
    result["nightsoulDisposed"] = next->players[0]->characters[0]->entities.empty();
  }
  {
    auto state = initial(data); auto entity = add(state, 991001, AreaType::Hands); auto* skill = skillOf(entity, EventName::Initiative);
    result["artifactTargets"] = targetIds(state, entity, skill);
    InitiativeSkillArgument event({state->players[0]->characters[1]}); auto next = action(state, entity, skill, event).state;
    result["equipment"] = JsonValue::Object{{"hands", static_cast<int>(next->players[0]->hands.size())}, {"equipped", next->players[0]->characters[1]->entities[0]->definition->id}};
  }
  {
    auto state = initial(data); auto entity = add(state, 991002, AreaType::Hands); auto* skill = skillOf(entity, EventName::Initiative);
    InitiativeSkillArgument event; auto next = action(state, entity, skill, event).state;
    result["support"] = JsonValue::Object{{"hands", static_cast<int>(next->players[0]->hands.size())}, {"supports", definitionIds(next->players[0]->supports)}};
  }
  {
    auto state = initial(data); auto entity = add(state, 991003, AreaType::Hands); auto* skill = skillOf(entity, EventName::Initiative);
    result["foodTargets"] = targetIds(state, entity, skill);
  }
  {
    auto state = initial(data); auto entity = add(state, 991001, AreaType::Hands); auto* skill = skillOf(entity, EventName::Initiative);
    setActiveCharacter(state, 0, state->players[0]->characters[1]->id);
    result["rotatedTargets"] = targetIds(state, entity, skill);
  }
  {
    auto state = initial(data); auto entity = add(state, 990010); auto* skill = skillOf(entity, EventName::OnCustomEvent);
    CustomEvent other{"test"};
    CustomEventEventArg different(state, entity, &other, JsonValue());
    auto* own = registry.findCustomEvent("fixture");
    if (!own) throw std::runtime_error("Fixture custom event was not registered");
    CustomEventEventArg event(state, entity, own, JsonValue());
    result["customOther"] = skill->filter(state, info(entity, skill), different);
    // Check an unbound slot through its actual action wrapper, and binding after definitions are built.
    auto outcome = skill->action(state, info(entity, skill), event);
    if (outcome.error) try { std::rethrow_exception(outcome.error); } catch (const std::exception& error) { result["unbound"] = error.what(); }
    else throw std::runtime_error("Unbound action unexpectedly succeeded");
    registry.lambdaSlots()->bind(999, [](LambdaInvocation&) { return JsonValue(); });
    if (skill->action(state, info(entity, skill), event).error) throw std::runtime_error("Late binding failed");
    result["customFilter"] = skill->filter(state, info(entity, skill), event);
  }
  {
    auto state = initial(data); auto* extension = data.extensions.values().front(); auto* skill = extension->skills.front();
    SkillInfo invocation; invocation.caller = state->players[0]->characters[0]; invocation.definition = skill; invocation.finalizeMode = FinalizeMode::Simple;
    PlayerEventArg event(state, 1); auto outcome = skill->action(state, invocation, event);
    if (outcome.error) std::rethrow_exception(outcome.error);
    result["extension"] = outcome.state->extensions.front().state;
  }
  {
    auto state = initial(data); auto entity = add(state, 991004); const SkillDefinition* skill = nullptr;
    for (auto* candidate : entity->definition->skills) if (candidate->skillType == SkillType::Technique) skill = candidate;
    if (!skill) throw std::runtime_error("Missing technique fixture");
    InitiativeSkillArgument event; auto next = action(state, entity, skill, event).state;
    result["technique"] = variables(next->players[0]->characters[0]->entities[0]->variables);
  }
  {
    auto state = initial(data); auto modifications = data.attachments.find(999001)->modifications(state, 0);
    auto values = JsonValue::array();
    for (const auto& value : modifications) {
      if (value.kind != AttachmentModification::Kind::IncreaseCardCost) throw std::runtime_error("Wrong attachment modification kind");
      values.push(JsonValue::Object{{"type", "increaseCardCost"}, {"value", value.value}});
    }
    result["attachment"] = values;
  }
  {
    auto state = initial(data); auto entity = add(state, 990005, AreaType::Summons); auto* skill = skillOf(entity, EventName::OnDamageOrHeal);
    DamageInfo damage; damage.type = damage::Anemo; damage.value = 1; damage.target = state->players[1]->characters[0]; damage.source = state->players[0]->characters[0];
    damage.targetAura = aura::Pyro;
    DamageOrHealEventArg event(state, damage, std::nullopt);
    result["swirlFilter"] = skill->filter(state, info(entity, skill), event);
    auto next = action(state, entity, skill, event).state;
    result["swirlHint"] = variables(next->players[0]->summons[0]->variables);
  }
  {
    auto state = initial(data); auto entity = add(state, 991003, AreaType::Hands); auto* skill = skillOf(entity, EventName::Initiative);
    InitiativeSkillArgument event({state->players[0]->characters[0]}); auto next = action(state, entity, skill, event).state;
    result["foodSatiated"] = definitionIds(next->players[0]->characters[0]->entities);
  }
  {
    auto state = initial(data); add(state, 990007); auto entity = add(state, 991004); auto* skill = skillOf(entity, EventName::OnBeforeAction);
    PlayerEventArg event(state, 1); auto outcome = action(state, entity, skill, event);
    auto* dispose = skillOf(entity, EventName::OnDispose); GameStatePointer final;
    for (const auto& emitted : outcome.emittedEvents) if (emitted.name == EventName::OnDispose) final = action(outcome.state, entity, dispose, *emitted.argument).state;
    if (!final) throw std::runtime_error("Missing technique disposal event");
    result["techniqueNightsoul"] = definitionIds(final->players[0]->characters[0]->entities);
  }
  {
    auto state = initial(data); auto entity = add(state, 991005, AreaType::Pile); add(state, 991005, AreaType::Hands);
    resetDice(state, 0, {2, 2, 2, 2}); auto* skill = skillOf(entity, EventName::OnActionPhase); EventArg event(state);
    result["blessingFilter"] = skill->filter(state, info(entity, skill), event);
    auto next = action(state, entity, skill, event).state;
    auto dice = JsonValue::array(); for (int die : next->players[0]->dice) dice.push(die);
    result["blessing"] = JsonValue::Object{{"hands", static_cast<int>(next->players[0]->hands.size())}, {"pile", static_cast<int>(next->players[0]->pile.size())}, {"supports", definitionIds(next->players[0]->supports)}, {"dice", dice}};
  }
  {
    auto state = initial(data); auto entity = add(state, 991006, AreaType::Hands); auto* skill = skillOf(entity, EventName::Initiative);
    InitiativeSkillArgument event; auto outcome = action(state, entity, skill, event);
    auto events = JsonValue::array(); for (const auto& emitted : outcome.emittedEvents) events.push(eventNameText(emitted.name));
    result["adventure"] = JsonValue::Object{{"variables", variables(outcome.state->players[0]->supports[0]->variables)}, {"events", events}};
  }
  {
    auto state = initial(data); auto entity = add(state, 990012, AreaType::Summons);
    auto descriptions = JsonValue::object();
    for (const auto& [key, getter] : entity->definition->descriptionDictionary) descriptions[key == "X" ? "text" : "hint"] = getter(state, entity->id);
    result["description"] = descriptions;
  }
  return result;
}
}  // namespace gitcg
