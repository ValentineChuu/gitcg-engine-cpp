// Every displayed counter of every board entity, by entity id (reference/bridge/entity_counters.mjs).
#pragma once

#include "core/json/json_value.h"
#include "core/state/game_state.h"

namespace gitcg {

JsonValue boardCountersById(const GameStatePointer& state);

}  // namespace gitcg
