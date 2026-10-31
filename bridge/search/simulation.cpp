#include "bridge/search/simulation.h"

namespace gitcg {
Simulation::Simulation(GameStatePointer state) : game_(std::move(state), *this, this) {
  game_.onStepStart = [this](const GameStatePointer& state) { stepStart = state; };
}
void Simulation::run() {
  scheduler_.spawn([this] {
    try { game_.start(); }
    catch (const Stopped&) {}
    catch (const std::exception& error) { failed = true; failure = error.what(); }
  });
  scheduler_.runUntilIdle();
  if (failed && failure.find("not translated") != std::string::npos) throw std::runtime_error(failure);
}
[[noreturn]] void Simulation::stop() { game_.terminate(); throw Stopped(); }
void Simulation::tick() { scheduler_.wake(scheduler_.current()); scheduler_.wait(); }
ActionAnswer Simulation::action(int who, const std::vector<ActionInfo>& actions) {
  if (beforeChoice) beforeChoice(who, "action");
  ActionAnswer answer = decide(who, actions); tick(); return answer;
}
int Simulation::chooseActive(int who, const std::vector<int>& candidates) {
  if (beforeChoice) beforeChoice(who, "chooseActive");
  int answer = candidates.front(); tick(); return answer;
}
std::vector<DiceType> Simulation::rerollDice(int who) {
  if (beforeChoice) beforeChoice(who, "rerollDice");
  tick(); return {};
}
std::vector<int> Simulation::switchHands(int who) {
  if (beforeChoice) beforeChoice(who, "switchHands");
  tick(); return {};
}
int Simulation::selectCard(int who, const std::vector<int>& candidates) {
  if (beforeChoice) beforeChoice(who, "selectCard");
  int answer = candidates.front(); tick(); return answer;
}
void Simulation::all(const std::function<void()>& first, const std::function<void()>& second) {
  int remaining = 2;
  std::exception_ptr error;
  Scheduler::Task* parent = scheduler_.current();
  auto finished = [&](Scheduler::Task& task) {
    if (!error && task.fiber->error()) error = task.fiber->error();
    if (--remaining == 0) scheduler_.wake(parent);
  };
  scheduler_.spawn(first, finished);
  scheduler_.spawn(second, finished);
  while (remaining) scheduler_.wait();
  if (error) std::rethrow_exception(error);
}
}  // namespace gitcg
