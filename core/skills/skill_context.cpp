// SkillContext: construction, closing a skill's events, and reading the state.

#include "core/skills/skill_context.h"

#include <algorithm>

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"
#include "core/definitions/extension_definition.h"
#include "core/definitions/game_data.h"
#include "core/mutations/state_mutations.h"
#include "core/rules/character_traits.h"
#include "core/rules/costs.h"
#include "core/rules/entity_lookup.h"
#include "core/values/engine_errors.h"

namespace gitcg {

SkillContext::SkillContext(GameStatePointer state, const SkillInfo& skillInfo, SkillArgument& eventArgument, const ContextOptions& options)
    : mutator_(std::move(state)), skillInfo_(skillInfo), eventArgument_(&eventArgument), options_(options) {}

SkillAction SkillContext::encapsulate(std::shared_ptr<const ContextOptions> options, SkillOperation operation) {
  return [options, operation](const GameStatePointer& state, const SkillInfo& skill, SkillArgument& argument) {
    SkillContext context(state, skill, argument, *options);
    std::exception_ptr error;
    try {
      operation(context);
      context.eventBoundary();
    } catch (...) {
      error = std::current_exception();
    }
    return context.terminate(error);
  };
}

SkillOutcome SkillContext::terminate(std::exception_ptr error) {
  SkillOutcome outcome;
  outcome.state = mutator_.state();
  outcome.emittedEvents = std::move(processedEvents_);
  outcome.causeDefeated = causeDefeated_;
  outcome.error = error;
  outcome.mainDamage = mainDamage_;
  return outcome;
}

std::shared_ptr<const SkillInfo> SkillContext::sharedSkillInfo() {
  if (!sharedSkillInfo_) sharedSkillInfo_ = std::make_shared<SkillInfo>(skillInfo_);
  return sharedSkillInfo_;
}

void SkillContext::eventBoundary() {
  std::vector<EventAndArgument> processed = preprocessEvents();
  processedEvents_.insert(processedEvents_.end(), processed.begin(), processed.end());
  currentEvents_ = EventList();
}

void SkillContext::emitEvent(EventName name, std::shared_ptr<SkillArgument> argument) {
  currentEvents_.push({name, std::move(argument)});
}

std::vector<EventAndArgument> SkillContext::preprocessEvents() {
  static const Name ALIVE = intern("alive");
  static const Name ENERGY = intern("energy");
  static const Name AURA = intern("aura");
  if (skillInfo_.finalizeMode == FinalizeMode::Simple) return currentEvents_.items();
  std::vector<EventAndArgument> otherEvents;
  std::vector<EventAndArgument> handCardEvents;
  std::vector<EventAndArgument> safeDamageEvents;
  std::vector<EventAndArgument> criticalDamageEvents;
  bool failed[2] = {false, false};
  // The list grows while it is walked (immune heals are appended), as in the JS loop.
  for (size_t index = 0; index < currentEvents_.items().size(); ++index) {
    EventAndArgument event = currentEvents_.items()[index];
    if (event.name == EventName::OnDamageOrHeal) {
      auto& damageEvent = static_cast<DamageOrHealEventArg&>(*event.argument);
      if (damageEvent.isDamageTypeDamage()) {
        DamageInfo info = damageEvent.damageInfo();
        if (!info.causeDefeated) {
          safeDamageEvents.push_back(event);
          continue;
        }
        auto zeroHealth = std::make_shared<ZeroHealthEventArg>(damageEvent.onTimeState, info, damageEvent.option);
        EventsAndDefeat inner = mutator_.handleInlineEvent(skillInfo_, EventName::ModifyZeroHealth, *zeroHealth);
        currentEvents_.append(inner.events);
        causeDefeated_ = causeDefeated_ || inner.causeDefeated;
        if (!zeroHealth->immuneSkill) {
          CharacterHandle defeated(this, info.target->id);
          if (defeated.alive()) {
            int who = defeated.who();
            const CharacterDefinition& definition = defeated.definition();
            modifyEntityVariable(mutator_.mutableState(), defeated.id(), ALIVE, 0, 2);
            Name energyName = definition.specialEnergy ? definition.specialEnergy->variableName : ENERGY;
            modifyEntityVariable(mutator_.mutableState(), defeated.id(), energyName, 0, 2);
            modifyEntityVariable(mutator_.mutableState(), defeated.id(), AURA, aura::None);
            gitcg::setPlayerFlag(mutator_.mutableState(), who, PlayerFlag::HasDefeated, true);
            removeRoundSkillLog(mutator_.mutableState(), defeated.definition().id, defeated.id());
            const auto& characters = state().players[who]->characters;
            bool anyAlive = std::any_of(characters.begin(), characters.end(), [](const CharacterPointer& c) { return isCharacterAlive(*c); });
            if (!anyAlive) failed[who] = true;
          }
          criticalDamageEvents.push_back(event);
          causeDefeated_ = true;
        } else {
          safeDamageEvents.push_back({EventName::OnDamageOrHeal, zeroHealth});
        }
      } else {
        otherEvents.push_back(event);
      }
    } else if (event.name == EventName::OnHandCardInserted) {
      auto& inserted = static_cast<HandCardInsertedEventArg&>(*event.argument);
      bool emit = inserted.overflowed;
      if (!emit) {
        EntityArea area = entityArea(inserted.card->id);
        emit = area.who == inserted.who && area.type == AreaType::Hands;
      }
      if (emit) handCardEvents.push_back(event);
    } else {
      otherEvents.push_back(event);
    }
  }
  std::vector<EventAndArgument> emitted;
  for (auto* group : {&otherEvents, &handCardEvents, &safeDamageEvents, &criticalDamageEvents}) {
    emitted.insert(emitted.end(), group->begin(), group->end());
  }
  if (failed[0] && failed[1]) {
    changePhase(mutator_.mutableState(), Phase::GameEnd);
  } else if (failed[0] || failed[1]) {
    changePhase(mutator_.mutableState(), Phase::GameEnd);
    setWinner(mutator_.mutableState(), failed[0] ? 1 : 0);
  }
  return emitted;
}

AnyHandle SkillContext::self() {
  return AnyHandle(this, skillInfo_.caller.id(), skillInfo_.caller.isCharacter());
}

int SkillContext::selfWho() {
  return entityArea(skillInfo_.caller.id()).who;
}

const PlayerState& SkillContext::player() {
  return *state().players[selfWho()];
}

const PlayerState& SkillContext::oppPlayer() {
  return *state().players[1 - selfWho()];
}

bool SkillContext::isMyTurn() {
  return state().currentTurn == selfWho();
}

AnyHandle SkillContext::get(int id) {
  AnyState found = getEntityById(state(), id);
  return AnyHandle(this, id, found.isCharacter());
}

EntityArea SkillContext::entityArea(int id) {
  return getEntityArea(state(), id);
}

std::vector<AnyHandle> SkillContext::handlesOf(const std::vector<AnyState>& states) {
  std::vector<AnyHandle> handles;
  for (const AnyState& found : states) handles.emplace_back(this, found.id(), found.isCharacter());
  return handles;
}

const JsonValue& SkillContext::getExtensionState() {
  if (!options_.associatedExtensionId) throw GameDataError("No associated extension registered");
  for (const ExtensionState& extension : state().extensions) {
    if (extension.definition->id == *options_.associatedExtensionId) return extension.state;
  }
  throw GameDataError("Associated extension not found");
}

void SkillContext::setExtensionState(const std::function<void(JsonValue&)>& setter) {
  JsonValue newState = getExtensionState();
  takeExtensionContainerReplaced();
  setter(newState);
  bool replacedContainer = takeExtensionContainerReplaced();
  mutateExtensionState(mutator_.mutableState(), *options_.associatedExtensionId, std::move(newState), replacedContainer);
}

int SkillContext::countOfSkill() {
  return countOfSkill(CharacterHandle(this, skillInfo_.caller.id()).definition().id, skillInfo_.definition->id);
}

int SkillContext::countOfSkill(int characterDefinitionId, double skillId) {
  for (const auto& [definitionId, skills] : player().roundSkillLog) {
    if (definitionId == characterDefinitionId) return static_cast<int>(std::count(skills.begin(), skills.end(), skillId));
  }
  return 0;
}

bool SkillContext::hasPhaseDamage(bool allPlayers, const std::function<bool(DamageOrHealEventArg&)>& filter) {
  int me = selfWho();
  std::vector<std::shared_ptr<DamageOrHealEventArg>> damages = state().players[me]->phaseDamageLog;
  if (allPlayers) {
    const auto& opponent = state().players[1 - me]->phaseDamageLog;
    damages.insert(damages.end(), opponent.begin(), opponent.end());
  }
  return std::any_of(damages.begin(), damages.end(), [&](const std::shared_ptr<DamageOrHealEventArg>& damage) {
    return damage->isDamageTypeDamage() && (!filter || filter(*damage));
  });
}

bool SkillContext::hasPhaseReaction(bool allPlayers, const std::function<bool(ReactionEventArg&)>& filter) {
  int me = selfWho();
  std::vector<std::shared_ptr<ReactionEventArg>> reactions = state().players[me]->phaseReactionLog;
  if (allPlayers) {
    const auto& opponent = state().players[1 - me]->phaseReactionLog;
    reactions.insert(reactions.end(), opponent.begin(), opponent.end());
  }
  return std::any_of(reactions.begin(), reactions.end(), [&](const std::shared_ptr<ReactionEventArg>& reactionEvent) {
    return !filter || filter(*reactionEvent);
  });
}

std::vector<EntityHandle> SkillContext::maxCostHands(int count, bool opponent, const std::function<bool(const EntityHandle&)>& filter) {
  const PlayerState& owner = opponent ? oppPlayer() : player();
  std::vector<std::pair<EntityHandle, int>> hands;
  for (const EntityPointer& card : owner.hands) {
    EntityHandle handle(this, card->id);
    if (filter && !filter(handle)) continue;
    hands.emplace_back(handle, -diceCostSizeOfCard(rawState(), *card));
  }
  std::stable_sort(hands.begin(), hands.end(), [](const auto& first, const auto& second) { return first.second < second.second; });
  std::vector<EntityHandle> result;
  for (size_t index = 0; index < hands.size() && static_cast<int>(index) < count; ++index) result.push_back(hands[index].first);
  return result;
}

bool SkillContext::isInInitialPile(const EntityHandle& card, bool opponent) {
  int definitionId = card.definition().id;
  const auto& initial = (opponent ? oppPlayer() : player()).initialPile;
  return std::any_of(initial.begin(), initial.end(), [&](const EntityDefinition* definition) { return definition->id == definitionId; });
}

int SkillContext::remainingSupportCount(bool opponent) {
  return state().config.maxSupportsCount - static_cast<int>((opponent ? oppPlayer() : player()).supports.size());
}

std::vector<const EntityDefinition*> SkillContext::allCardDefinitions(const std::function<bool(const EntityDefinition&)>& filter) {
  std::vector<const EntityDefinition*> result;
  for (const EntityDefinition* definition : state().data->entities.values()) {
    if (!definition->obtainable) continue;
    if (!filter || filter(*definition)) result.push_back(definition);
  }
  return result;
}

bool SkillContext::isSelfDisposeCausedByDefeatedHeuristically(const std::function<bool(DamageOrHealEventArg&)>& extraCondition) {
  auto& disposeEvent = static_cast<DisposeEventArg&>(*eventArgument_);
  if (disposeEvent.from.type != AreaType::Characters) return false;
  int characterId = disposeEvent.from.characterId;
  if (CharacterHandle(this, characterId).alive()) return false;
  return hasPhaseDamage(true, [&](DamageOrHealEventArg& damage) {
    DamageInfo info = damage.damageInfo();
    return info.causeDefeated && info.target->id == characterId && (!extraCondition || extraCondition(damage));
  });
}

void SkillContext::abortPreview() {
  if (isPreview()) throw PreviewAbortedError();
}

void SkillContext::callSnippet(const std::string& name, SkillArgument* argument) {
  auto snippet = options_.snippets.find(name);
  if (snippet == options_.snippets.end()) throw GameDataError("Snippet " + name + " not found");
  SkillArgument* previous = eventArgument_;
  eventArgument_ = argument;
  snippet->second(*this);
  eventArgument_ = previous;
}

void SkillContext::emitCustomEvent(const CustomEvent* event, JsonValue argument) {
  AnyState self = getEntityById(state(), skillInfo_.caller.id());
  emitEvent(EventName::OnCustomEvent, std::make_shared<CustomEventEventArg>(rawState(), self, event, std::move(argument)));
}

void SkillContext::handleCustomEventInline(const CustomEvent* event, JsonValue argument, const SkillArgument* rawArgument) {
  AnyState self = getEntityById(state(), skillInfo_.caller.id());
  CustomEventEventArg eventArg(rawState(), self, event, std::move(argument));
  eventArg.rawArg = rawArgument;
  EventsAndDefeat inner = mutator_.handleInlineEvent(skillInfo_, EventName::OnCustomEvent, eventArg);
  callAndEmit(inner.events);
  causeDefeated_ = causeDefeated_ || inner.causeDefeated;
}

}  // namespace gitcg
