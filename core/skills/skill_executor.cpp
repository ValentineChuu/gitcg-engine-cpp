#include "core/skills/skill_executor.h"

#include <algorithm>
#include <exception>

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"
#include "core/definitions/game_data.h"
#include "core/events/action_event_args.h"
#include "core/mutations/state_mutations.h"
#include "core/mutations/mutation_observer.h"
#include "core/rules/costs.h"
#include "core/rules/entity_lookup.h"
#include "core/rules/initiative_skills.h"
#include "core/skills/skill_outcome.h"

namespace gitcg {

namespace {

std::shared_ptr<SkillInfo> makeSkillInfo(AnyState caller, const SkillDefinition* definition) {
  auto info = std::make_shared<SkillInfo>();
  info->caller = std::move(caller);
  info->definition = definition;
  return info;
}

}  // namespace

bool SkillExecutor::ended() const {
  return mutator_.state()->phase == Phase::GameEnd;
}

SkillExecutor::Executed SkillExecutor::executeSkill(const SkillInfo& skill, SkillArgument& argument) {
  if (ended()) return {};
  if (isCharacterInitiativeSkill(skill)) {
    int who = getEntityArea(*mutator_.state(), skill.caller.id()).who;
    setPlayerFlag(mutator_.mutableState(), who, PlayerFlag::CanPlunging, false);
  }
  SkillInfo running = skill;
  running.environment = environment_;
  GameStatePointer before = mutator_.state();
  ObservedSkill observation;
  SkillOutcome outcome = skill.definition->action(mutator_.state(), running, argument);
  observation.finish(outcome.state != before);
  mutator_.resetState(outcome.state);
  if (outcome.error) std::rethrow_exception(outcome.error);
  return {std::move(outcome.emittedEvents), outcome.causeDefeated};
}

void SkillExecutor::finalizeSkill(const SkillInfo& skill, SkillArgument& argument) {
  static const Name ENERGY = intern("energy");
  static const Name MAX_ENERGY = intern("maxEnergy");
  static const Name ALIVE = intern("alive");
  Executed executed = executeSkill(skill, argument);
  if (ended()) return;
  if (skill.caller.isCharacter() && skill.definition->triggerOn == EventName::Initiative) {
    CharacterPointer character = getEntityById(*mutator_.state(), skill.caller.id()).character();
    // Intentional in the core: the skill counts under the definition the caller had when the skill started.
    pushRoundSkillLog(mutator_.mutableState(), skill.caller.character()->definition->id, skill.caller.id(), skill.definition->id);
    if (skill.definition->initiativeSkillConfig->gainEnergy && character->variables.valueOr(ALIVE, 0) != 0 &&
        !character->definition->specialEnergy) {
      double energy = character->variables.valueOr(ENERGY, 0);
      double newEnergy = std::min(energy + 1, character->variables.valueOr(MAX_ENERGY, 0));
      modifyEntityVariable(mutator_.mutableState(), character->id, ENERGY, newEnergy, 1);
    }
  }
  handleEvents(executed.emittedEvents);
  // Switching after a defeated active character, only when this very skill caused a defeat.
  if (ended() || !executed.causeDefeated) return;
  bool savedSwitching[2] = {mutator_.state()->players[0]->defeatedSwitching, mutator_.state()->players[1]->defeatedSwitching};
  CharacterPointer chosen[2];
  CharacterPointer activeBefore[2];
  std::array<PlayerPointer, 2> players = mutator_.state()->players;
  auto chooseFor = [&](int who) {
    CharacterPointer active = activeCharacter(*players[who]);
    if (active->variables.valueOr(ALIVE, 0) != 0) return;
    activeBefore[who] = active;
    setPlayerFlag(mutator_.mutableState(), who, PlayerFlag::DefeatedSwitching, true);
    chosen[who] = mutator_.chooseActive(who);
  };
  if (concurrency_) {
    concurrency_->all([&] { chooseFor(0); }, [&] { chooseFor(1); });
  } else {
    chooseFor(0);
    chooseFor(1);
  }
  int currentTurn = mutator_.state()->currentTurn;
  for (int who : {currentTurn, 1 - currentTurn}) {
    if (!chosen[who]) continue;
    handleEvents(mutator_.switchActive(who, chosen[who]).items());
  }
  for (int who = 0; who < 2; ++who) setPlayerFlag(mutator_.mutableState(), who, PlayerFlag::DefeatedSwitching, savedSwitching[who]);
}

void SkillExecutor::handleEvents(const std::vector<EventAndArgument>& events) {
  for (const EventAndArgument& event : events) {
    if (ended()) return;
    switch (event.name) {
      case EventName::RequestReroll: {
        auto& request = static_cast<RerollRequestArg&>(*event.argument);
        mutator_.reroll(request.who, request.times);
        break;
      }
      case EventName::RequestSwitchHands: {
        auto& request = static_cast<RequestArg&>(*event.argument);
        handleEvents(mutator_.switchHands(request.who).items());
        break;
      }
      case EventName::RequestSelectCard: {
        auto& request = static_cast<SelectCardRequestArg&>(*event.argument);
        handleEvents(mutator_.selectCard(request.who, request.via, request.info).items());
        handleEvent(EventName::OnSelectCard, std::make_shared<SelectCardEventArg>(mutator_.state(), request.who, request.info));
        break;
      }
      case EventName::RequestUseSkill: handleRequestUseSkill(event); break;
      case EventName::RequestPlayCard: handleRequestPlayCard(event); break;
      case EventName::RequestAdventure: handleRequestAdventure(event); break;
      case EventName::RequestTriggerEndPhaseSkill: {
        auto& request = static_cast<TriggerEndPhaseSkillRequestArg&>(*event.argument);
        for (const SkillDefinition* skill : request.requestedEntity->definition->skills) {
          if (skill->triggerOn != EventName::OnEndPhase) continue;
          auto info = makeSkillInfo(request.requestedEntity, skill);
          info->requestBy = request.via;
          EventArg eventArg(mutator_.state());
          if (!skill->filter(mutator_.state(), *info, eventArg)) continue;
          finalizeSkill(*info, eventArg);
        }
        break;
      }
      default: broadcast(event); break;
    }
  }
}

void SkillExecutor::broadcast(const EventAndArgument& event) {
  auto& argument = static_cast<EventArg&>(*event.argument);
  std::vector<CallerAndSkill> listeners;
  // A disposed entity also hears its own disposal.
  if (event.name == EventName::OnDispose) {
    auto& disposeEvent = static_cast<DisposeEventArg&>(argument);
    for (const SkillDefinition* skill : disposeEvent.disposedEntity()->definition->skills) {
      if (skill->triggerOn == event.name) listeners.push_back({disposeEvent.entity, skill});
    }
  }
  std::vector<CallerAndSkill> others = allSkills(*mutator_.state(), event.name);
  listeners.insert(listeners.end(), others.begin(), others.end());
  for (const CallerAndSkill& listener : listeners) {
    auto info = makeSkillInfo(listener.caller, listener.skill);
    argument.currentSkillInfo = info.get();
    if (!listener.skill->filter(mutator_.state(), *info, argument)) continue;
    finalizeSkill(*info, argument);
  }
}

void SkillExecutor::handleRequestUseSkill(const EventAndArgument& event) {
  auto& request = static_cast<UseSkillRequestArg&>(*event.argument);
  const PlayerState& player = *mutator_.state()->players[request.who];
  std::vector<CallerAndSkill> available = initiativeSkillsOfPlayer(player, true);
  if (isSkillDisabled(*activeCharacter(player))) return;
  auto found = std::find_if(available.begin(), available.end(),
                            [&](const CallerAndSkill& entry) { return entry.skill->id == request.requestingSkillId; });
  if (found == available.end()) return;
  ChargedPlunging attack = chargedPlunging(*found->skill, player);
  auto info = makeSkillInfo(found->caller, found->skill);
  info->requestBy = request.via;
  info->charged = attack.charged;
  info->plunging = attack.plunging;
  info->prepared = request.asPrepared;
  EntityArea callerArea = getEntityArea(*mutator_.state(), found->caller.id());
  handleEvent(EventName::OnBeforeUseSkill, std::make_shared<UseSkillEventArg>(mutator_.state(), callerArea, info));
  InitiativeSkillArgument noTargets;
  finalizeSkill(*info, noTargets);
  handleEvent(EventName::OnUseSkill, std::make_shared<UseSkillEventArg>(mutator_.state(), callerArea, info));
}

void SkillExecutor::handleRequestPlayCard(const EventAndArgument& event) {
  auto& request = static_cast<PlayCardRequestArg&>(*event.argument);
  bool played = false;
  // A card created only to be played by a selection leaves the hand again however this ends.
  struct DisposeUnplayed {
    SkillExecutor* executor;
    PlayCardRequestArg* request;
    bool* played;
    ~DisposeUnplayed() {
      if (!request->viaSelect || *played) return;
      if (std::uncaught_exceptions() == 0) {
        removeEntity(executor->mutator_.mutableState(), request->card->id, request->who);
        return;
      }
      // Already failing (JS would wrap both errors in a SuppressedError): don't throw from a destructor.
      try {
        removeEntity(executor->mutator_.mutableState(), request->card->id, request->who);
      } catch (...) {
      }
    }
  } disposeUnplayed{this, &request, &played};

  const SkillDefinition* skillDefinition = playSkillOfCard(*request.card->definition);
  if (!skillDefinition) return;
  auto info = makeSkillInfo(request.card, skillDefinition);
  info->requestBy = request.via;
  // Resolve the targets.
  std::optional<InitiativeSkillArgument> targets;
  if (request.target.rule == PlayCardTarget::Rule::List) {
    InitiativeSkillArgument argument(request.target.targets);
    if (skillDefinition->filter(mutator_.state(), *info, argument)) targets = argument;
  } else {
    std::vector<InitiativeSkillArgument> candidates;
    for (InitiativeSkillArgument& candidate : playCardTargetCandidates(mutator_.state(), request.who, request.card, *info)) {
      if (skillDefinition->filter(mutator_.state(), *info, candidate)) candidates.push_back(candidate);
    }
    if (request.target.rule == PlayCardTarget::Rule::SkipIfRequired) {
      auto empty = std::find_if(candidates.begin(), candidates.end(), [](const InitiativeSkillArgument& c) { return c.targets.empty(); });
      if (empty != candidates.end()) targets = *empty;
    } else if (!candidates.empty()) {
      if (request.target.rule == PlayCardTarget::Rule::First) {
        targets = candidates[0];
      } else {
        targets = candidates[static_cast<size_t>(mutator_.stepRandom() % static_cast<int64_t>(candidates.size()))];
      }
    }
  }
  if (!targets) return;
  ModifiedHandCard modified = applyAttachmentModifications(mutator_.state(), *request.card);
  auto playCardInfo = std::make_shared<ActionInfo>();
  playCardInfo->type = ActionType::PlayCard;
  playCardInfo->who = request.who;
  playCardInfo->skill = info;
  playCardInfo->targets = targets->targets;
  playCardInfo->willBeEffectless = modified.willBeEffectless;
  playCardInfo->cost = modified.requiredCost;
  playCardInfo->originalCost = modified.requiredCost;
  playCardInfo->fast = modified.shouldFast;
  playCardInfo->originalFast = modified.shouldFast;
  if (!request.viaSelect) handleEvent(EventName::OnBeforePlayCard, std::make_shared<PlayCardEventArg>(mutator_.state(), playCardInfo));
  if (playCardInfo->willBeEffectless) {
    removeEntity(mutator_.mutableState(), request.card->id, request.who, RemoveReason::EventCardPlayNoEffect);
  } else {
    finalizeSkill(*info, *targets);
  }
  played = true;
  if (!request.viaSelect) handleEvent(EventName::OnPlayCard, std::make_shared<PlayCardEventArg>(mutator_.state(), playCardInfo));
}

void SkillExecutor::handleRequestAdventure(const EventAndArgument& event) {
  static const Name ADVENTURE_SPOT = intern("adventureSpot");
  static const Name EXP = intern("exp");
  auto& request = static_cast<RequestArg&>(*event.argument);
  const auto& supports = mutator_.state()->players[request.who]->supports;
  auto spot = std::find_if(supports.begin(), supports.end(), [](const EntityPointer& support) { return support->definition->hasTag(ADVENTURE_SPOT); });
  if (spot != supports.end()) {
    EntityPointer current = *spot;
    modifyEntityVariable(mutator_.mutableState(), current->id, EXP, current->variables.valueOr(EXP, 0) + 1);
    handleEvent(EventName::OnAdventure, std::make_shared<EntityEventArg>(mutator_.state(), current));
    return;
  }
  SelectCardInfo info;
  info.kind = SelectCardInfo::Kind::RequestPlayCard;
  info.target.rule = PlayCardTarget::Rule::SkipIfRequired;
  for (const EntityDefinition* definition : mutator_.state()->data->entities.values()) {
    if (definition->hasTag(ADVENTURE_SPOT)) info.cards.push_back(definition);
  }
  handleEvents(mutator_.selectCard(request.who, request.via, info).items());
  handleEvent(EventName::OnSelectCard, std::make_shared<SelectCardEventArg>(mutator_.state(), request.who, info));
}

}  // namespace gitcg
