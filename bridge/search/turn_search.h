// Distinct lines of a single player's turn, in breadth-first order and with the Node engine-play budget.
#pragma once
#include "core/events/action_info.h"
#include "core/state/game_state.h"
namespace gitcg {
JsonValue healthSnapshot(const GameStatePointer& state);
JsonValue healthDelta(const JsonValue& base, const GameStatePointer& state);
struct TurnLine { JsonValue change; bool usedCard; };
struct TurnSearchResult {
  std::vector<TurnLine> lines;
  int plays = 0;
  bool capped = false;
  int stoppedAtRandom = 0;
};
struct TurnSearchOptions {
  std::optional<std::vector<int>> firstMoves;
  int budget = 200;
  bool replies = false;
  bool cards = true;
  JsonValue base;
  std::optional<std::vector<ActionInfo>> actions;
};
TurnSearchResult searchTurn(const GameStatePointer& start, int player, TurnSearchOptions options = {});
}
