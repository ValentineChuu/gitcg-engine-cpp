#include "bridge/search/end_of_round.h"
#include "bridge/search/engine_work.h"
#include "bridge/search/simulation.h"
#include "bridge/search/turn_search.h"
#include "bridge/session/redeal.h"
namespace gitcg {
JsonValue endOfRound(const GameStatePointer& state, int who, double seed, const JsonValue* opponentCards) {
  auto start = redealHiddenCards(state, who, seed, opponentCards);
  JsonValue base = healthSnapshot(start);
  Simulation game(start);
  GameStatePointer final;
  game.beforeChoice = [&](int seat, const char*) {
    if (game.state()->roundNumber > start->roundNumber) { final = game.state(); game.stop(); }
  };
  game.decide = [&](int seat, const std::vector<ActionInfo>& actions) {
    int index = -1;
    for (size_t i = 0; i < actions.size(); ++i) if (actions[i].validity == 0) {
      if (index == -1) index = i;
      if (actions[i].type == ActionType::DeclareEnd) { index = i; break; }
    }
    return ActionAnswer{index, actions.at(index).autoSelectedDice};
  };
  ++engineWork.endOfRoundCopies;
  game.run();
  bool ended = !final;
  if (!final) final = game.state();
  JsonValue change = healthDelta(base, final);
  for (auto& side : change.asArray()) for (auto& pair : side.asArray()) {
    JsonValue entry = JsonValue::object(); entry["health"] = pair.asArray()[0]; entry["defeated"] = pair.asArray()[1]; pair = std::move(entry);
  }
  JsonValue result = JsonValue::object(); result["change"] = std::move(change);
  if (ended) { result["ended"] = true; result["winner"] = !game.failed && final->winner ? JsonValue(*final->winner) : JsonValue(); }
  return result;
}
}
