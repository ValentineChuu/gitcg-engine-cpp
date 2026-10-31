#include "bridge/search/turn_search.h"
#include <algorithm>
#include <set>
#include "bridge/search/simulation.h"
#include "bridge/search/turn_line_rules.h"
#include "core/expose/expose_state.h"
#include "core/json/json_text.h"

namespace gitcg {
JsonValue healthSnapshot(const GameStatePointer& state) {
  JsonValue result = JsonValue::array();
  for (const auto& player : state->players) {
    JsonValue side = JsonValue::array();
    for (const auto& c : player->characters) side.push(JsonValue::Array{c->variables.valueOr(intern("health"), 0), !c->variables.valueOr(intern("alive"), 0)});
    result.push(std::move(side));
  }
  return result;
}
JsonValue healthDelta(const JsonValue& base, const GameStatePointer& state) {
  JsonValue now = healthSnapshot(state);
  for (size_t seat = 0; seat < 2; ++seat) for (size_t i = 0; i < now.asArray()[seat].asArray().size(); ++i) {
    auto& pair = now.asArray()[seat].asArray()[i].asArray();
    const auto& before = base.asArray()[seat].asArray()[i].asArray();
    pair[0] = pair[0].asNumber() - before[0].asNumber();
    pair[1] = pair[1].asBool() && !before[1].asBool();
  }
  return now;
}
namespace {
struct StepResult {
  GameStatePointer final;
  GameStatePointer next;
  std::vector<ActionInfo> actions;
};
StepResult playStep(const GameStatePointer& state, int player, std::optional<int> index) {
  Simulation game(state);
  StepResult result;
  bool answered = false, endsTurn = false, done = false;
  auto finish = [&] { done = true; game.stop(); };
  game.beforeChoice = [&](int seat, const char* kind) {
    if (std::string_view(kind) != "action" && (seat != player || !answered)) {
      result.final = game.state(); finish();
    }
  };
  game.decide = [&](int seat, const std::vector<ActionInfo>& actions) -> ActionAnswer {
    if (seat == player) {
      if (!index) { result.actions = actions; finish(); }
      if (!answered) {
        answered = true;
        const auto& chosen = actions.at(*index); endsTurn = !chosen.fast;
        return {*index, chosen.autoSelectedDice};
      }
      if (endsTurn) result.final = game.state();
      else { result.next = game.stepStart; result.actions = actions; }
      finish();
    }
    result.final = game.state(); finish();
    return {};
  };
  game.run();
  if (!done) result.final = game.state();
  return result;
}
JsonValue strip(JsonValue value) {
  if (value.isArray()) for (auto& item : value.asArray()) item = strip(std::move(item));
  else if (value.isObject()) {
    std::erase_if(value.asObject(), [](const auto& p) { return p.first == "id" || p.first == "cardId"; });
    for (auto& [key, item] : value.asObject()) item = strip(std::move(item));
  }
  return value;
}
std::string fingerprint(const GameStatePointer& state, int who) {
  JsonValue view = strip(exposeState(who, state));
  for (auto& side : view["player"].asArray()) {
    auto& hand = side["handCard"].asArray();
    std::stable_sort(hand.begin(), hand.end(), [](const auto& a, const auto& b) { return a.at("definitionId").asInt() < b.at("definitionId").asInt(); });
  }
  // The full serialized view is an exact key; hashing it first would only save key storage.
  return writeJson(view);
}
}  // namespace
TurnSearchResult searchTurn(const GameStatePointer& start, int player, TurnSearchOptions options) {
  TurnSearchResult found;
  JsonValue base = options.base.isNull() ? healthSnapshot(start) : options.base;
  auto actions = options.actions;
  if (!actions) { actions = playStep(start, player, std::nullopt).actions; ++found.plays; }
  struct Node { GameStatePointer state; std::vector<ActionInfo> actions; bool root = false; bool usedCard = false; };
  std::vector<Node> frontier{{start, std::move(*actions), true, false}};
  std::set<std::string> seen;
  while (!frontier.empty() && !found.capped) {
    std::vector<Node> next;
    for (const auto& node : frontier) {
      std::vector<int> moves;
      if (node.root && options.firstMoves) {
        for (int i : *options.firstMoves) if (i >= 0 && static_cast<size_t>(i) < node.actions.size() && node.actions[i].validity == 0) moves.push_back(i);
      } else moves = branchingMoves(node.actions, node.state);
      for (int index : moves) {
        const auto& action = node.actions[index];
        if (options.replies && cardIsLeftOutOfReplies(action)) continue;
        if (!options.cards && action.type == ActionType::PlayCard) continue;
        if (found.plays >= options.budget) { found.capped = true; break; }
        auto result = playStep(node.state, player, index); ++found.plays;
        bool usedCard = node.usedCard || action.type == ActionType::PlayCard;
        if (result.final) { found.lines.push_back({healthDelta(base, result.final), usedCard}); continue; }
        if (endsLine(action)) { ++found.stoppedAtRandom; found.lines.push_back({healthDelta(base, result.next), usedCard}); continue; }
        if (!seen.insert(fingerprint(result.next, player)).second) continue;
        next.push_back({std::move(result.next), std::move(result.actions), false, usedCard});
      }
      if (found.capped) break;
    }
    frontier = std::move(next);
  }
  return found;
}
}  // namespace gitcg
