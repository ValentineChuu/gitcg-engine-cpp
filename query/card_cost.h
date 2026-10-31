// The two cost paths used by special:diceCost: static cost and executable attachment modifications.
#pragma once
#include "core/state/game_state.h"
namespace gitcg::query {
double cardDiceCost(const GameState& state, const EntityPointer& card);
}  // namespace gitcg::query
