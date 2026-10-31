// Base event arguments (core/src/base/skill.ts EventArg, PlayerEventArg, ModifyRollEventArg). Every event argument
// keeps the state at the moment it was raised (`onTimeState`); skills reading through it see that moment.
#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/definitions/event_name.h"
#include "core/events/skill_info.h"
#include "core/state/game_state.h"
#include "core/values/game_enums.h"

namespace gitcg {

class EventArg : public SkillArgument {
 public:
  explicit EventArg(GameStatePointer state) : onTimeState(std::move(state)) {}

  GameStatePointer onTimeState;
  const SkillInfo* currentSkillInfo = nullptr;  // set by whoever is about to run a skill on this argument

 protected:
  const AnyState& caller() const;
};

class PlayerEventArg : public EventArg {
 public:
  PlayerEventArg(GameStatePointer state, int player) : EventArg(std::move(state)), who(player) {}
  int who;
};

class ModifyRollEventArg : public PlayerEventArg {
 public:
  using PlayerEventArg::PlayerEventArg;
  void fixDice(DiceType type, int count);
  void addRerollCount(int count) { extraRerollCount += count; }

  std::vector<DiceType> fixedDice;
  int extraRerollCount = 0;
};

// An event or request and its argument, as skills emit them.
struct EventAndArgument {
  EventName name;
  std::shared_ptr<SkillArgument> argument;
};

}  // namespace gitcg
