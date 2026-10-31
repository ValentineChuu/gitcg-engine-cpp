// The query runtime, returning immutable state handles in the core's response order (AGPL-3.0).
#pragma once

#include "core/state/game_state.h"
#include "query/expression.h"

namespace gitcg {
std::vector<AnyState> runQuery(const GameState& state, int who, const QueryExpression& query);
inline std::vector<AnyState> runQuery(const GameState& state, int who, const JsonValue& expression) {
  return runQuery(state, who, QueryExpression(expression));
}
}  // namespace gitcg
