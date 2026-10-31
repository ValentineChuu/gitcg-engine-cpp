#include "bridge/search/opponent_replies.h"
#include <map>
#include "bridge/search/engine_work.h"
#include "bridge/search/reply_guess.h"
#include "bridge/search/simulation.h"
#include "bridge/search/turn_line_rules.h"
#include "bridge/search/turn_search.h"
#include "bridge/session/redeal.h"
namespace gitcg {
namespace {
JsonValue replyAfter(const GameStatePointer& start, int who, int index, const ActionInfo& action, const char* cardSource) {
  Simulation game(start);
  bool answered = false, stopped = false, done = false;
  JsonValue result;
  auto finish = [&](const char* next) {
    done = true; result = JsonValue::object(); result["next"] = next;
    if (stopped) result["stopped"] = true;
    game.stop();
  };
  game.beforeChoice = [&](int player, const char* kind) {
    if (std::string_view(kind) == "action") return;
    finish(player == who || stopped ? "self" : "other");
  };
  game.decide = [&](int player, const std::vector<ActionInfo>& actions) -> ActionAnswer {
    if (!answered && player == who) { answered = true; return {index, action.autoSelectedDice}; }
    if (player == who && !stopped) {
      for (size_t i = 0; i < actions.size(); ++i) if (actions[i].type == ActionType::DeclareEnd && actions[i].validity == 0) {
        stopped = true; return {static_cast<int>(i), {}};
      }
    }
    if (player == who) finish("self");
    TurnSearchOptions options;
    options.budget = 150; options.replies = true; options.cards = cardSource != nullptr;
    options.base = healthSnapshot(start); options.actions = actions;
    auto found = searchTurn(game.stepStart, 1 - who, std::move(options));
    ++engineWork.replies.searches; engineWork.replies.plays += found.plays + 1; engineWork.replies.overBudget += found.capped;
    result = JsonValue::object(); result["next"] = stopped ? "self" : "opponent";
    if (stopped) result["stopped"] = true;
    result["capped"] = found.capped;
    JsonValue lines = JsonValue::array();
    for (const auto& line : found.lines) {
      JsonValue entry = JsonValue::object(); entry["mine"] = line.change.asArray()[who]; entry["usedCard"] = line.usedCard;
      lines.push(std::move(entry));
    }
    result["lines"] = std::move(lines);
    if (cardSource) result["cards"] = cardSource;
    done = true; game.stop(); return {};
  };
  game.run();
  if (!done) { result = JsonValue::object(); result["next"] = stopped ? "self" : "finished"; }
  return result;
}
}
JsonValue opponentReplies(const GameStatePointer& state, int who, const std::vector<ActionInfo>& actions, double seed,
                          const JsonValue& view, const std::vector<int>& opponentPlayed) {
  auto cards = opponentCardsFor(state, who, seed, view, opponentPlayed);
  const char* source = view.at("opponentDeck").asString() == "open" ? "open" : !cards.isNull() ? "guessed" : nullptr;
  auto start = redealHiddenCards(state, who, seed, &cards);
  PlayerState& opponent = editable(editable(start).players[1 - who]);
  for (int& die : opponent.dice) die = dice::Omni;
  std::map<std::string, JsonValue> byOutcome;
  JsonValue result = JsonValue::array();
  for (size_t i = 0; i < actions.size(); ++i) {
    const auto& action = actions[i];
    if (action.validity != 0) { result.push({}); continue; }
    std::string key = action.type == ActionType::ElementalTuning ? "tune:" + std::to_string(action.result)
      : action.type == ActionType::PlayCard ? "card:" + cardOutcomeKey(action) : "action:" + std::to_string(i);
    auto it = byOutcome.find(key);
    if (it == byOutcome.end()) it = byOutcome.emplace(key, replyAfter(start, who, i, action, source)).first;
    result.push(it->second);
  }
  return result;
}
}
