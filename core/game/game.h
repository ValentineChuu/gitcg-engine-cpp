// One game from a state to its end (core/src/game.ts Game, with our patches): the phase loop, the actions offered
// each turn, and carrying out the chosen one. Previews are optional and off by default.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "core/events/action_event_args.h"
#include "core/game/player_io.h"
#include "core/mutations/mutator.h"
#include "core/mutations/notifications.h"
#include "core/skills/skill_executor.h"

namespace gitcg {

// An offered action and the modifier object that will apply its cost changes for real if it is chosen.
struct OfferedAction {
  ActionInfo info;
  std::shared_ptr<ModifyActionEventArg> eventArg;
};

// Thrown into a suspended game to end it without a result (terminate); start() returns nullopt.
class GameTerminated : public std::exception {};

class Game : private MutatorIo {
 public:
  Game(GameStatePointer initialState, PlayerIo& io, Concurrency* concurrency = nullptr);

  // Plays until the end; returns the winner (nullopt for a draw). Throws when the game fails.
  std::optional<int> start();
  void terminate() { terminated_ = true; }
  bool terminated() const { return terminated_; }

  const GameStatePointer& state() const { return mutator_.state(); }
  bool enablePreview = false;

  // Patch 6: called at the top of each step with the state the step starts from (a copy restarts there).
  std::function<void(const GameStatePointer&)> onStepStart;
  // Patch 5: called just before the per-phase damage and reaction logs are cleared.
  std::function<void(const GameStatePointer&)> beforeClearPhaseLogs;

 private:
  class EndInterrupt : public std::exception {};

  Phase initHands();
  Phase initActives();
  Phase rollPhase();
  Phase actionPhase();
  Phase endPhase();
  std::vector<OfferedAction> availableActions();
  void performAction(int who, const OfferedAction& offered, const std::vector<DiceType>& usedDice);
  void finishGame(std::optional<int> winner);

  void executeSkill(const SkillInfo& skill, SkillArgument& argument);
  void handleEvent(EventName name, std::shared_ptr<SkillArgument> argument);
  void handleEvents(const std::vector<EventAndArgument>& events);
  void interruptIfGameEnded();
  void checkNotTerminated() const;
  // With a person's seat (core/mutations/notifications.h): game.ts setPlayerStatus, and notifyOne for both seats.
  void showStatus(int who, PlayerStatus status);
  void notifyBoth(int who, const char* kind, JsonValue ownValue, JsonValue otherValue);

  // MutatorIo: the mutator's player requests, checked like the core's rpc wrappers.
  std::vector<int> switchHands(int who) override;
  std::vector<DiceType> reroll(int who) override;
  int selectCard(int who, const std::vector<int>& candidateDefinitionIds) override;
  int chooseActive(int who, const std::vector<int>& candidateIds) override;

  Mutator mutator_;
  PlayerIo& io_;
  Concurrency* concurrency_;
  bool terminated_ = false;
  std::optional<int> result_;
};

}  // namespace gitcg
