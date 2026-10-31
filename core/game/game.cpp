// Game: the phase loop and the phases other than the action phase.

#include "core/game/game.h"

#include <algorithm>

#include "core/mutations/state_mutations.h"
#include "core/rules/dice_order.h"
#include "core/rules/entity_lookup.h"
#include "core/values/engine_errors.h"

namespace gitcg {

namespace {

// Runs both branches; without a fiber scheduler they run one after the other.
void runBoth(Concurrency* concurrency, const std::function<void()>& first, const std::function<void()>& second) {
  if (concurrency) {
    concurrency->all(first, second);
  } else {
    first();
    second();
  }
}

}  // namespace

Game::Game(GameStatePointer initialState, PlayerIo& io, Concurrency* concurrency)
    : mutator_(std::move(initialState), this), io_(io), concurrency_(concurrency) {}

void Game::checkNotTerminated() const {
  if (terminated_) throw GameTerminated();
}

void Game::interruptIfGameEnded() {
  if (mutator_.state()->phase == Phase::GameEnd) throw EndInterrupt();
}

void Game::executeSkill(const SkillInfo& skill, SkillArgument& argument) {
  SkillExecutor(mutator_, SkillEnvironment::Normal, concurrency_).finalizeSkill(skill, argument);
  interruptIfGameEnded();
}

void Game::handleEvent(EventName name, std::shared_ptr<SkillArgument> argument) {
  SkillExecutor(mutator_, SkillEnvironment::Normal, concurrency_).handleEvent(name, std::move(argument));
  interruptIfGameEnded();
}

void Game::handleEvents(const std::vector<EventAndArgument>& events) {
  SkillExecutor(mutator_, SkillEnvironment::Normal, concurrency_).handleEvents(events);
  interruptIfGameEnded();
}

std::optional<int> Game::start() {
  try {
    while (!terminated_) {
      if (onStepStart) onStepStart(mutator_.state());
      Phase newPhase = Phase::GameEnd;
      switch (mutator_.state()->phase) {
        case Phase::InitHands: newPhase = initHands(); break;
        case Phase::InitActives: newPhase = initActives(); break;
        case Phase::Roll: newPhase = rollPhase(); break;
        case Phase::Action: newPhase = actionPhase(); break;
        case Phase::End: newPhase = endPhase(); break;
        case Phase::GameEnd: newPhase = Phase::GameEnd; break;
      }
      if (mutator_.state()->phase == Phase::GameEnd) {
        finishGame(mutator_.state()->winner);
        return result_;
      }
      changePhase(mutator_.mutableState(), newPhase);
      clearRemovedEntities(mutator_.mutableState());
      if (beforeClearPhaseLogs) beforeClearPhaseLogs(mutator_.state());
      clearPhaseLogs(mutator_.mutableState());
    }
  } catch (const EndInterrupt&) {
    finishGame(mutator_.state()->winner);
    return result_;
  } catch (const GameIoError& error) {
    finishGame(1 - error.who);
    return result_;
  }
  throw GameTerminated();
}

void Game::finishGame(std::optional<int> winner) {
  if (mutator_.state()->phase != Phase::GameEnd) {
    changePhase(mutator_.mutableState(), Phase::GameEnd);
    if (winner && winner != mutator_.state()->winner) setWinner(mutator_.mutableState(), *winner);
  }
  result_ = winner;
  terminated_ = true;
}

Phase Game::initHands() {
  for (int who : {0, 1}) handleEvents(mutator_.drawCardsPlain(who, mutator_.state()->config.initialHandsCount).items());
  EventList events[2];
  runBoth(concurrency_, [&] { events[0] = mutator_.switchHands(0); }, [&] { events[1] = mutator_.switchHands(1); });
  EventList all;
  all.append(events[0]);
  all.append(events[1]);
  handleEvents(all.items());
  return Phase::InitActives;
}

Phase Game::initActives() {
  CharacterPointer chosen[2];
  runBoth(concurrency_, [&] { chosen[0] = mutator_.chooseActive(0); }, [&] { chosen[1] = mutator_.chooseActive(1); });
  // Patch 3: both actives are set before either player's switch events run.
  SwitchActiveOptions notAnAction;
  EventList first = mutator_.switchActive(0, chosen[0], notAnAction);
  EventList second = mutator_.switchActive(1, chosen[1], notAnAction);
  handleEvents(first.items());
  handleEvents(second.items());
  handleEvent(EventName::OnBattleBegin, std::make_shared<EventArg>(mutator_.state()));
  stepRound(mutator_.mutableState());
  return Phase::Roll;
}

Phase Game::rollPhase() {
  handleEvent(EventName::OnRoundBegin, std::make_shared<EventArg>(mutator_.state()));
  struct RollParameters {
    std::vector<DiceType> fixed;
    int count = 1;
  } parameters[2];
  for (int who : {0, 1}) {
    auto rollModifier = std::make_shared<ModifyRollEventArg>(mutator_.state(), who);
    handleEvent(EventName::ModifyRoll, rollModifier);
    parameters[who] = {rollModifier->fixedDice, 1 + rollModifier->extraRerollCount};
  }
  auto rollFor = [&](int who) {
    const RollParameters& roll = parameters[who];
    std::vector<DiceType> initial = roll.fixed;
    int randomCount = std::max(0, mutator_.state()->config.initialDiceCount - static_cast<int>(roll.fixed.size()));
    std::vector<DiceType> rolled = mutator_.randomDice(randomCount);
    initial.insert(initial.end(), rolled.begin(), rolled.end());
    resetDice(mutator_.mutableState(), who, sortDice(*mutator_.state()->players[who], initial));
    mutator_.reroll(who, roll.count);
  };
  runBoth(concurrency_, [&] { rollFor(0); }, [&] { rollFor(1); });
  return Phase::Action;
}

Phase Game::endPhase() {
  handleEvent(EventName::OnEndPhase, std::make_shared<EventArg>(mutator_.state()));
  int currentTurn = mutator_.state()->currentTurn;
  for (int who : {currentTurn, 1 - currentTurn}) handleEvents(mutator_.drawCardsPlain(who, 2).items());
  handleEvent(EventName::OnRoundEnd, std::make_shared<EventArg>(mutator_.state()));
  for (int who : {0, 1}) {
    setPlayerFlag(mutator_.mutableState(), who, PlayerFlag::HasDefeated, false);
    setPlayerFlag(mutator_.mutableState(), who, PlayerFlag::CanPlunging, false);
  }
  clearRoundLogs(mutator_.mutableState());
  stepRound(mutator_.mutableState());
  return mutator_.state()->roundNumber >= mutator_.state()->config.maxRoundsCount ? Phase::GameEnd : Phase::Roll;
}

std::vector<int> Game::switchHands(int who) {
  interruptIfGameEnded();
  checkNotTerminated();
  std::vector<int> removed = io_.switchHands(who);
  checkNotTerminated();
  std::vector<int> sorted = removed;
  std::sort(sorted.begin(), sorted.end());
  if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end()) throw GameIoError(who, "Invalid response of switchHands: duplicate card ids");
  return removed;
}

std::vector<DiceType> Game::reroll(int who) {
  interruptIfGameEnded();
  checkNotTerminated();
  std::vector<DiceType> dice = io_.rerollDice(who);
  checkNotTerminated();
  return dice;
}

int Game::selectCard(int who, const std::vector<int>& candidateDefinitionIds) {
  interruptIfGameEnded();
  checkNotTerminated();
  int selected = io_.selectCard(who, candidateDefinitionIds);
  checkNotTerminated();
  if (std::find(candidateDefinitionIds.begin(), candidateDefinitionIds.end(), selected) == candidateDefinitionIds.end()) {
    throw GameIoError(who, "Selected card not in candidates");
  }
  return selected;
}

int Game::chooseActive(int who, const std::vector<int>& candidateIds) {
  interruptIfGameEnded();
  checkNotTerminated();
  int chosen = io_.chooseActive(who, candidateIds);
  checkNotTerminated();
  if (std::find(candidateIds.begin(), candidateIds.end(), chosen) == candidateIds.end()) {
    throw GameIoError(who, "Invalid active character id");
  }
  return chosen;
}

}  // namespace gitcg
