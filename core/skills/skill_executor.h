// Runs skills and the events they cause (core/src/skill_executor.ts): a skill's action, then (for character
// skills) the round log and energy, then its events in order (requests carried out here), then the switch of any
// defeated active character.
#pragma once

#include <functional>
#include <memory>

#include "core/events/event_list.h"
#include "core/events/skill_info.h"
#include "core/mutations/mutator.h"

namespace gitcg {

// Runs two branches as JS Promise.all would: both started, each resumed when its answer arrives. Games driven by
// fibers interleave them; without one, the branches simply run one after the other.
class Concurrency {
 public:
  virtual ~Concurrency() = default;
  virtual void all(const std::function<void()>& first, const std::function<void()>& second) = 0;
};

class SkillExecutor {
 public:
  SkillExecutor(Mutator& mutator, SkillEnvironment environment, Concurrency* concurrency)
      : mutator_(mutator), environment_(environment), concurrency_(concurrency) {}

  void finalizeSkill(const SkillInfo& skill, SkillArgument& argument);
  void handleEvents(const std::vector<EventAndArgument>& events);
  void handleEvent(EventName name, std::shared_ptr<SkillArgument> argument) { handleEvents({{name, std::move(argument)}}); }

 private:
  struct Executed {
    std::vector<EventAndArgument> emittedEvents;
    bool causeDefeated = false;
  };
  Executed executeSkill(const SkillInfo& skill, SkillArgument& argument);
  void handleRequestUseSkill(const EventAndArgument& event);
  void handleRequestPlayCard(const EventAndArgument& event);
  void handleRequestAdventure(const EventAndArgument& event);
  void broadcast(const EventAndArgument& event);
  bool ended() const;

  Mutator& mutator_;
  SkillEnvironment environment_;
  Concurrency* concurrency_;
};

}  // namespace gitcg
