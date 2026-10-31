// What running one skill action gives back (core/src/base/skill.ts SkillResult with the new state).
#pragma once

#include <exception>
#include <memory>
#include <vector>

#include "core/events/damage_info.h"
#include "core/events/event_arg.h"
#include "core/state/game_state.h"

namespace gitcg {

struct SkillOutcome {
  GameStatePointer state;
  std::vector<EventAndArgument> emittedEvents;
  bool causeDefeated = false;
  std::exception_ptr error;  // thrown by the action; the state up to the throw is still returned
  std::shared_ptr<const DamageInfo> mainDamage;
};

}  // namespace gitcg
