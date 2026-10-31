#include "bridge/session/game_session.h"
#include <map>
#include <set>
#include "bridge/search/follow_ups.h"
#include "bridge/search/opponent_replies.h"
#include "bridge/search/reply_guess.h"
#include "bridge/search/turn_line_rules.h"
#include "bridge/session/redeal.h"
namespace gitcg {
void GameSession::computeReplies(int who, const JsonValue& indices) {
  if (who < 0 || who > 1 || !replyContext_[who])
    throw std::runtime_error("game " + std::to_string(gameId_) + ": player " + std::to_string(who) + " has no decision waiting for replies");
  const auto& context = *replyContext_[who];
  std::set<int> chosen;
  std::vector<int> order;
  JsonValue checked = JsonValue::array();
  for (const auto& value : indices.asArray()) if (chosen.insert(value.asInt()).second) { order.push_back(value.asInt()); checked.push(value); }
  auto actions = context.actions;
  for (size_t i = 0; i < actions.size(); ++i) if (!chosen.contains(i)) actions[i].validity = 1;
  auto replies = opponentReplies(context.state, who, actions, context.seed, context.view, playedCards_[1 - who]);
  auto cards = opponentCardsFor(context.state, who, context.seed, context.view, playedCards_[1 - who]);
  auto start = redealHiddenCards(context.state, who, context.seed, &cards);
  std::map<std::string, JsonValue> searched;
  auto outcome = [&](int i) {
    const auto& action = context.actions[i];
    if (action.type == ActionType::ElementalTuning) return "tune:" + std::to_string(action.result);
    if (action.type == ActionType::PlayCard) return "card:" + cardOutcomeKey(action);
    return "move:" + std::to_string(i);
  };
  for (int index : order) {
    if (index < 0 || static_cast<size_t>(index) >= replies.asArray().size() || replies.asArray()[index].isNull()) continue;
    const auto& action = context.actions[index];
    std::string key = action.type == ActionType::PlayCard ? "card:" + cardOutcomeKey(action) : "move:" + std::to_string(index);
    auto it = searched.find(key);
    if (it == searched.end()) it = searched.emplace(key, followUps(start, who, context.actions, index)).first;
    // Node reuses the same reply object for equal outcomes (including all tunings to one die).
    for (int i : order) if (i >= 0 && static_cast<size_t>(i) < replies.asArray().size() && !replies.asArray()[i].isNull() && outcome(i) == outcome(index))
      replies.asArray()[i]["followUps"] = it->second;
  }
  JsonValue result = JsonValue::object(); result["type"] = "replies"; result["game"] = gameId_; result["who"] = who;
  result["replies"] = std::move(replies); result["checked"] = std::move(checked); emit_(result);
}
}
