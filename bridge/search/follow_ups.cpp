#include "bridge/search/follow_ups.h"
#include <algorithm>
#include "bridge/search/engine_work.h"
#include "bridge/search/turn_search.h"
namespace gitcg {
JsonValue followUps(const GameStatePointer& start, int who, const std::vector<ActionInfo>& actions, int index, int budget) {
  TurnSearchOptions options;
  options.firstMoves = std::vector<int>{index}; options.budget = budget; options.actions = actions;
  auto found = searchTurn(start, who, std::move(options));
  ++engineWork.followUps.searches; engineWork.followUps.plays += found.plays; engineWork.followUps.overBudget += found.capped;
  size_t count = start->players[1 - who]->characters.size();
  JsonValue mostLost = JsonValue::array(), defeated = JsonValue::array();
  double mostDamage = 0;
  for (size_t i = 0; i < count; ++i) {
    double lost = 0; bool dead = false;
    for (const auto& line : found.lines) {
      const auto& pair = line.change.asArray()[1 - who].asArray()[i].asArray();
      lost = std::max(lost, -pair[0].asNumber()); dead = dead || pair[1].asBool();
    }
    mostLost.push(lost); defeated.push(dead);
  }
  for (const auto& line : found.lines) {
    double total = 0;
    for (const auto& pair : line.change.asArray()[1 - who].asArray()) total -= std::min(0.0, pair.asArray()[0].asNumber());
    mostDamage = std::max(mostDamage, total);
  }
  JsonValue result = JsonValue::object(); result["mostLost"] = std::move(mostLost); result["defeated"] = std::move(defeated);
  result["mostDamage"] = mostDamage; result["lines"] = static_cast<int>(found.lines.size());
  result["stoppedAtRandom"] = found.stoppedAtRandom; result["capped"] = found.capped;
  return result;
}
}
