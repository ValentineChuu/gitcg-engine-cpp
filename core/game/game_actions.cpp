// Game: the action phase, the actions offered each turn, and carrying out the chosen one (core/src/game.ts
// actionPhase and availableActions, optional core/src/preview.ts previews, utils.ts
// applyAutoSelectedDiceToAction).

#include <algorithm>

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"
#include "core/game/game.h"
#include "core/preview/action_preview.h"
#include "core/mutations/state_mutations.h"
#include "core/rules/character_traits.h"
#include "core/rules/costs.h"
#include "core/rules/dice_choice.h"
#include "core/rules/dice_order.h"
#include "core/rules/entity_lookup.h"
#include "core/rules/initiative_skills.h"
#include "core/values/engine_errors.h"

namespace gitcg {

namespace {

const DiceRequirement& voidOneCost() {
  static const DiceRequirement cost = [] {
    DiceRequirement value;
    value.set(dice::Void, 1);
    return value;
  }();
  return cost;
}

// The first replaceAction skill whose filter passes, for a skill called from a character area.
std::shared_ptr<SkillInfo> findReplaceAction(const GameStatePointer& state, EventArg& eventArg) {
  for (const CallerAndSkill& entry : allSkills(*state, EventName::ReplaceAction)) {
    if (getEntityArea(*state, entry.caller.id()).type != AreaType::Characters) continue;
    auto info = std::make_shared<SkillInfo>();
    info->caller = entry.caller;
    info->definition = entry.skill;
    if (entry.skill->filter(state, *info, eventArg)) return info;
  }
  return nullptr;
}

ActionInfo withAutoSelectedDice(ActionInfo action, const PlayerState& player) {
  if (action.validity != action_validity::Valid) return action;
  const CharacterDefinition& elementSource =
      action.type == ActionType::SwitchActive ? *action.to->definition : *activeCharacter(player)->definition;
  std::set<DiceType> activeCharacterDice{elementOfCharacter(elementSource)};
  std::set<DiceType> disallowed;
  if (action.type == ActionType::ElementalTuning) {
    disallowed.insert(action.result);
    disallowed.insert(dice::Omni);
  }
  std::vector<DiceType> chosen = chooseDiceValue(action.cost, player.dice, playerUsefulDice(player), activeCharacterDice, disallowed);
  if (!checkDice(action.cost, chosen)) {
    action.validity = action_validity::NoDice;
    return action;
  }
  static const Name ENERGY = intern("energy");
  if (activeCharacter(player)->variables.valueOr(ENERGY, 0) < action.cost.countOf(dice::Energy)) {
    action.validity = action_validity::NoEnergy;
    return action;
  }
  if (player.legendUsed && action.cost.countOf(dice::Legend) > 0) {
    action.validity = action_validity::NoLegend;
    return action;
  }
  action.autoSelectedDice = chosen;
  return action;
}

}  // namespace

std::vector<OfferedAction> Game::availableActions() {
  const GameStatePointer state = mutator_.state();
  int who = state->currentTurn;
  const PlayerState& player = *state->players[who];
  const CharacterPointer& active = activeCharacter(player);
  std::vector<ActionInfo> actions;

  bool skillDisabled = isSkillDisabled(*active);
  for (const CallerAndSkill& entry : initiativeSkillsOfPlayer(player)) {
    ChargedPlunging attack = chargedPlunging(*entry.skill, player);
    auto skillInfo = std::make_shared<SkillInfo>();
    skillInfo->caller = entry.caller;
    skillInfo->definition = entry.skill;
    skillInfo->charged = attack.charged;
    skillInfo->plunging = attack.plunging;
    const InitiativeSkillConfig& config = *entry.skill->initiativeSkillConfig;
    ActionInfo base;
    base.type = ActionType::UseSkill;
    base.who = who;
    base.skill = skillInfo;
    base.fast = base.originalFast = config.shouldFast;
    base.cost = base.originalCost = config.requiredCost;
    if (skillDisabled) {
      base.validity = action_validity::Disabled;
      actions.push_back(base);
      continue;
    }
    std::vector<InitiativeSkillArgument> allTargets = config.getTarget(state, *skillInfo);
    if (allTargets.empty()) {
      base.validity = action_validity::NoTarget;
      actions.push_back(base);
      continue;
    }
    for (InitiativeSkillArgument& targets : allTargets) {
      ActionInfo action = base;
      if (!entry.skill->filter(state, *skillInfo, targets)) {
        action.validity = action_validity::ConditionNotMet;
      } else {
        action.targets = targets.targets;
        action.validity = action_validity::Valid;
      }
      actions.push_back(action);
    }
  }

  DiceType defaultTunedTo = elementOfCharacter(*active->definition);
  for (const EntityPointer& card : player.hands) {
    ModifiedHandCard modified = applyAttachmentModifications(state, *card);
    const SkillDefinition* playSkill = playSkillOfCard(*card->definition);
    if (playSkill) {
      auto skillInfo = std::make_shared<SkillInfo>();
      skillInfo->caller = card;
      skillInfo->definition = playSkill;
      ActionInfo base;
      base.type = ActionType::PlayCard;
      base.who = who;
      base.skill = skillInfo;
      base.cost = base.originalCost = modified.requiredCost;
      base.fast = base.originalFast = modified.shouldFast;
      base.willBeEffectless = modified.willBeEffectless;
      std::vector<InitiativeSkillArgument> allTargets = playCardTargetCandidates(state, who, card, *skillInfo);
      if (allTargets.empty()) {
        base.validity = action_validity::NoTarget;
        actions.push_back(base);
      } else {
        for (InitiativeSkillArgument& targets : allTargets) {
          ActionInfo action = base;
          if (!playSkill->filter(state, *skillInfo, targets)) {
            action.validity = action_validity::ConditionNotMet;
          } else {
            action.targets = targets.targets;
            action.validity = action_validity::Valid;
          }
          actions.push_back(action);
        }
      }
    }
    ActionInfo tuning;
    tuning.type = ActionType::ElementalTuning;
    tuning.who = who;
    tuning.card = card;
    tuning.result = modified.tunedToType.value_or(defaultTunedTo);
    tuning.fast = tuning.originalFast = true;
    tuning.cost = tuning.originalCost = voidOneCost();
    tuning.validity = modified.disableTuning ? action_validity::Disabled : action_validity::Valid;
    actions.push_back(tuning);
  }

  for (const CharacterPointer& character : player.characters) {
    if (!isCharacterAlive(*character) || character->id == active->id) continue;
    ActionInfo switching;
    switching.type = ActionType::SwitchActive;
    switching.who = who;
    switching.from = active;
    switching.to = character;
    switching.cost = switching.originalCost = voidOneCost();
    switching.validity = action_validity::Valid;
    actions.push_back(switching);
  }

  ActionInfo declareEnd;
  declareEnd.type = ActionType::DeclareEnd;
  declareEnd.who = who;
  declareEnd.validity = action_validity::Valid;
  actions.push_back(declareEnd);

  std::vector<OfferedAction> offered;
  for (const ActionInfo& action : actions) {
    auto realEventArg = std::make_shared<ModifyActionEventArg>(state, std::make_shared<ActionInfo>(action));
    ActionInfo priced = modifyAndPreview(state, action, enablePreview);
    offered.push_back({withAutoSelectedDice(priced, player), realEventArg});
  }
  return offered;
}

Phase Game::actionPhase() {
  if (mutator_.state()->previousPhase == Phase::Roll) handleEvent(EventName::OnActionPhase, std::make_shared<EventArg>(mutator_.state()));
  int who = mutator_.state()->currentTurn;
  auto player = [&]() -> const PlayerState& { return *mutator_.state()->players[who]; };
  if (player().declaredEnd) {
    switchTurn(mutator_.mutableState());
  } else if (player().skipNextTurn) {
    setPlayerFlag(mutator_.mutableState(), who, PlayerFlag::SkipNextTurn, false);
    switchTurn(mutator_.mutableState());
  } else {
    handleEvent(EventName::OnBeforeAction, std::make_shared<PlayerEventArg>(mutator_.state(), who));
    setPlayerFlag(mutator_.mutableState(), who, PlayerFlag::CanCharged, player().dice.size() % 2 == 0);
    if (beforeClearPhaseLogs) beforeClearPhaseLogs(mutator_.state());
    clearPhaseLogs(mutator_.mutableState());
    PlayerEventArg replaceArgument(mutator_.state(), who);
    std::shared_ptr<SkillInfo> replaced = findReplaceAction(mutator_.state(), replaceArgument);
    if (replaced) {
      executeSkill(*replaced, replaceArgument);
      switchTurn(mutator_.mutableState());
    } else {
      std::vector<OfferedAction> offered = availableActions();
      std::vector<ActionInfo> infos;
      for (const OfferedAction& action : offered) infos.push_back(action.info);
      interruptIfGameEnded();
      checkNotTerminated();
      ActionAnswer answer = io_.action(who, infos);
      checkNotTerminated();
      if (answer.chosenActionIndex < 0 || answer.chosenActionIndex >= static_cast<int>(offered.size())) {
        throw GameIoError(who, "User chosen index out of range");
      }
      performAction(who, offered[answer.chosenActionIndex], answer.usedDice);
    }
  }
  if (mutator_.state()->players[0]->declaredEnd && mutator_.state()->players[1]->declaredEnd) {
    for (int player : {0, 1}) setPlayerFlag(mutator_.mutableState(), player, PlayerFlag::DeclaredEnd, false);
    return Phase::End;
  }
  return Phase::Action;
}

void Game::performAction(int who, const OfferedAction& offered, const std::vector<DiceType>& usedDice) {
  static const Name ENERGY = intern("energy");
  const ActionInfo& action = offered.info;
  auto player = [&]() -> const PlayerState& { return *mutator_.state()->players[who]; };
  if (action.validity != action_validity::Valid) throw GameIoError(who, "User-selected action is invalid");
  std::vector<DiceType> selected = player().dice;
  for (DiceType type : usedDice) {
    auto found = std::find(selected.begin(), selected.end(), type);
    if (found == selected.end()) throw GameIoError(who, "Selected dice doesn't found in player");
    selected.erase(found);
  }
  for (EventName stage : {EventName::ModifyAction0, EventName::ModifyAction1, EventName::ModifyAction2, EventName::ModifyAction3,
                          EventName::ModifyAction4}) {
    handleEvent(stage, offered.eventArg);
  }
  if (!checkDice(action.cost, usedDice)) throw GameIoError(who, "Selected dice doesn't meet requirement");
  if (action.type == ActionType::ElementalTuning && !usedDice.empty() && (usedDice[0] == dice::Omni || usedDice[0] == action.result)) {
    throw GameIoError(who, "Elemental tuning cannot use omni dice or active character's element");
  }
  // unexpectedInsufficientDice "skipConsume": modifiers may already have taken chosen dice.
  std::vector<DiceType> operating = player().dice;
  bool consume = true;
  for (DiceType type : usedDice) {
    auto found = std::find(operating.begin(), operating.end(), type);
    if (found == operating.end()) {
      consume = false;
      break;
    }
    operating.erase(found);
  }
  if (consume) {
    resetDice(mutator_.mutableState(), who, operating);
    int requiredEnergy = action.cost.countOf(dice::Energy);
    if (requiredEnergy > 0) {
      const CharacterPointer& active = activeCharacter(player());
      double energy = active->variables.valueOr(ENERGY, 0);
      if (energy < requiredEnergy) throw GameIoError(who, "Active character does not have enough energy");
      modifyEntityVariable(mutator_.mutableState(), active->id, ENERGY, energy - requiredEnergy);
    }
    if (action.cost.countOf(dice::Legend) > 0) {
      if (player().legendUsed) throw GameIoError(who, "Player has already used legend");
      setPlayerFlag(mutator_.mutableState(), who, PlayerFlag::LegendUsed, true);
    }
  }
  auto actionShared = std::make_shared<ActionInfo>(action);
  switch (action.type) {
    case ActionType::UseSkill: {
      EntityArea callerArea = getEntityArea(*mutator_.state(), activeCharacter(player())->id);
      handleEvent(EventName::OnBeforeUseSkill, std::make_shared<UseSkillEventArg>(mutator_.state(), callerArea, action.skill));
      InitiativeSkillArgument targets(action.targets);
      executeSkill(*action.skill, targets);
      handleEvent(EventName::OnUseSkill, std::make_shared<UseSkillEventArg>(mutator_.state(), callerArea, action.skill));
      break;
    }
    case ActionType::PlayCard: {
      const EntityPointer& card = action.skill->caller.entity();
      handleEvent(EventName::OnBeforePlayCard, std::make_shared<PlayCardEventArg>(mutator_.state(), actionShared));
      if (action.willBeEffectless) {
        removeEntity(mutator_.mutableState(), card->id, who, RemoveReason::EventCardPlayNoEffect);
      } else {
        InitiativeSkillArgument targets(action.targets);
        executeSkill(*action.skill, targets);
      }
      handleEvent(EventName::OnPlayCard, std::make_shared<PlayCardEventArg>(mutator_.state(), actionShared));
      break;
    }
    case ActionType::SwitchActive: {
      SwitchActiveOptions options;
      options.fast = action.fast;
      handleEvents(mutator_.switchActive(who, action.to, options).items());
      break;
    }
    case ActionType::ElementalTuning: {
      EntityArea from{AreaType::Hands, who, 0, action.card->id};
      auto tuned = std::make_shared<DisposeEventArg>(mutator_.state(), action.card, RemoveReason::ElementalTuning, from, nullptr);
      removeEntity(mutator_.mutableState(), action.card->id, who, RemoveReason::ElementalTuning);
      std::vector<DiceType> dice = player().dice;
      dice.push_back(action.result);
      resetDice(mutator_.mutableState(), who, sortDice(player(), dice));
      handleEvent(EventName::OnDispose, tuned);
      break;
    }
    case ActionType::DeclareEnd: {
      setPlayerFlag(mutator_.mutableState(), who, PlayerFlag::DeclaredEnd, true);
      handleEvent(EventName::OnDeclareEnd, std::make_shared<PlayerEventArg>(mutator_.state(), who));
      break;
    }
  }
  handleEvent(EventName::OnAction, std::make_shared<ActionEventArg>(mutator_.state(), actionShared));
  if (!action.fast) switchTurn(mutator_.mutableState());
}

}  // namespace gitcg
