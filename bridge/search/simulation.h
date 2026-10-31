// Synchronous caller interface to a search copy. Its immediate answers still yield between concurrent player
// branches, as resolved Node promises do (notably the two initial rolls before either reroll answer resumes).
#pragma once
#include "bridge/fibers/scheduler.h"
#include "core/game/game.h"

namespace gitcg {
class Simulation : private PlayerIo, private Concurrency {
 public:
  using Decide = std::function<ActionAnswer(int, const std::vector<ActionInfo>&)>;
  using BeforeChoice = std::function<void(int, const char*)>;
  explicit Simulation(GameStatePointer state);
  void run();
  [[noreturn]] void stop();
  GameStatePointer state() const { return game_.state(); }
  GameStatePointer stepStart;
  Decide decide;
  BeforeChoice beforeChoice;
  bool failed = false;
  std::string failure;
 private:
  class Stopped : public std::exception {};
  void tick();
  ActionAnswer action(int who, const std::vector<ActionInfo>& actions) override;
  int chooseActive(int who, const std::vector<int>& candidates) override;
  std::vector<DiceType> rerollDice(int who) override;
  std::vector<int> switchHands(int who) override;
  int selectCard(int who, const std::vector<int>& candidates) override;
  void all(const std::function<void()>& first, const std::function<void()>& second) override;
  Scheduler scheduler_;
  Game game_;
};
}  // namespace gitcg
