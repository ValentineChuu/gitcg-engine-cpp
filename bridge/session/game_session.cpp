#include "bridge/session/game_session.h"
#include "core/mutations/state_mutations.h"

#include <algorithm>

#include "bridge/session/entity_counters.h"
#include "bridge/session/redeal.h"
#include "bridge/search/end_of_round.h"
#include "bridge/search/engine_work.h"
#include "bridge/search/follow_ups.h"
#include "bridge/search/opponent_replies.h"
#include "bridge/search/reply_guess.h"
#include "bridge/search/turn_line_rules.h"
#include <map>
#include <set>
#include "core/definitions/entity_definition.h"
#include "core/expose/expose_state.h"
#include "core/rules/character_traits.h"
#include "core/values/engine_errors.h"

namespace gitcg {

GameSession::GameSession(int gameId, GameStatePointer initialState, Scheduler& scheduler, Emit emit, QueueDecision queueDecision,
                         std::optional<ForcedAnswer> forcedFirstAnswer)
    : gameId_(gameId), scheduler_(scheduler), emit_(std::move(emit)), queueDecision_(std::move(queueDecision)),
      forcedFirstAnswer_(std::move(forcedFirstAnswer)) {
  game_ = std::make_unique<Game>(std::move(initialState), static_cast<PlayerIo&>(*this), static_cast<Concurrency*>(this));
  game_->onStepStart = [this](const GameStatePointer& state) { stepStartState_ = state; };
  game_->beforeClearPhaseLogs = [this](const GameStatePointer& state) { combatLog_.scan(state); };
}

GameSession::~GameSession() = default;

void GameSession::configure(bool previews, bool replies, const JsonValue* replyViews, double seed, bool privileged,
                            std::optional<int> humanSeat, bool compactNotifications) {
  compactNotifications_ = compactNotifications;
  previews_ = previews || replies;
  replies_ = replies;
  privileged_ = privileged;
  if (humanSeat && (*humanSeat < 0 || *humanSeat > 1)) throw std::runtime_error("humanSeat must be 0 or 1");
  humanSeat_ = humanSeat;
  if (humanSeat) {
    notifications_ = std::make_unique<NotificationSink>(*humanSeat, [this](JsonValue notification) {
      sendNotification(std::move(notification));
    });
  }
  replySeed_ = seed;
  game_->enablePreview = previews_ || humanSeat.has_value();  // a person's board shows previews (game_session.mjs)
  for (int who : {0, 1}) {
    if (!replies) replyViews_[who] = {};
    else if (replyViews && !replyViews->isNull()) replyViews_[who] = replyViews->asArray().at(who);
    else { replyViews_[who] = JsonValue::object(); replyViews_[who]["opponentDeck"] = "hidden"; }
  }
}

void GameSession::recordInitialEntities(const std::vector<std::pair<EntityArea, int>>& created) {
  if (!notifications_) return;
  NotificationSink* saved = notificationSink;
  notificationSink = notifications_.get();
  for (const auto& [area, id] : created) recordEntityCreated(game_->state(), area, id);
  notificationSink = saved;
}

void GameSession::start() {
  Scheduler::Task* task = scheduler_.spawn([this] { run(); });
  task->notifications = notifications_.get();
}

std::unique_ptr<GameSession> GameSession::copy(int copyId, int who, const JsonValue& response, double seed,
                                             const JsonValue* opponentCards) {
  if (who < 0 || who > 1 || !pending_[who] || pending_[who]->response ||
      pending_[who]->request.at("$case").asString() != "action" || !stepStartState_ ||
      stepStartState_->phase != Phase::Action) {
    throw std::runtime_error("game " + std::to_string(gameId_) + ": player " + std::to_string(who) +
                             " has no pending action decision to copy");
  }
  auto result = std::make_unique<GameSession>(copyId, redealHiddenCards(stepStartState_, who, seed, opponentCards),
                                             scheduler_, emit_, queueDecision_,
                                             ForcedAnswer{who, pending_[who]->request.at("value").at("action"), response});
  result->configure(previews_, false, nullptr, 0, false, std::nullopt);
  return result;
}

void GameSession::run() {
  try {
    std::optional<int> winner = game_->start();
    if (stopped_) return;
    const GameStatePointer& state = game_->state();
    combatLog_.scan(state);
    JsonValue defeated = JsonValue::array();
    for (const PlayerPointer& player : state->players) {
      defeated.push(static_cast<int>(std::count_if(player->characters.begin(), player->characters.end(),
                                                   [](const CharacterPointer& character) { return !isCharacterAlive(*character); })));
    }
    JsonValue message = JsonValue::object();
    message["type"] = "finished";
    message["game"] = gameId_;
    message["winner"] = winner ? JsonValue(*winner) : JsonValue();
    message["rounds"] = state->roundNumber;
    message["defeated"] = std::move(defeated);
    message["reactions"] = combatLog_.reactionsJson();
    message["reactionCauses"] = combatLog_.reactionCausesJson();
    message["damageDealt"] = combatLog_.damageDealtJson();
    finish(std::move(message));
  } catch (const FiberCancelled&) {
    done_ = true;
  } catch (const GameTerminated&) {
    done_ = true;
  } catch (const std::exception& error) {
    if (stopped_) return;
    JsonValue message = JsonValue::object();
    message["type"] = "failed";
    message["game"] = gameId_;
    message["message"] = std::string(error.what());
    finish(std::move(message));
  }
}

void GameSession::finish(JsonValue message) {
  done_ = true;
  flushHeldNotification();
  emit_(message);
}

// In compact mode a notification whose mutations only open and close events (handleEvent, which the board uses for
// the indentation of its history, nothing it animates) is held; its mutations go in front of the
// next notification's, whose state is newer. Anything held is sent before a decision and at the end.
void GameSession::sendNotification(JsonValue notification) {
  if (compactNotifications_) {
    auto& mutations = notification["mutation"].asArray();
    // A notification without mutations is a plain state the board compares the next one against (the first one at
    // the game's start above all): never held.
    bool onlyMarkers = !mutations.empty() && std::all_of(mutations.begin(), mutations.end(), [](const JsonValue& entry) {
      return entry.at("mutation").at("$case").asString() == "handleEvent";
    });
    if (heldNotification_) {
      auto& held = (*heldNotification_)["mutation"].asArray();
      mutations.insert(mutations.begin(), held.begin(), held.end());
      heldNotification_.reset();
    }
    if (onlyMarkers) {
      heldNotification_ = std::move(notification);
      return;
    }
  }
  JsonValue message = JsonValue::object();
  message["type"] = "notify";
  message["game"] = gameId_;
  message["who"] = *humanSeat_;
  message["notification"] = std::move(notification);
  emit_(message);
}

void GameSession::flushHeldNotification() {
  if (!heldNotification_) return;
  JsonValue held = std::move(*heldNotification_);
  heldNotification_.reset();
  compactNotifications_ = false;  // sent as it is
  sendNotification(std::move(held));
  compactNotifications_ = true;
}

JsonValue GameSession::askPlayer(int who, JsonValue request) {
  if (forcedFirstAnswer_) {
    ForcedAnswer forced = std::move(*forcedFirstAnswer_);
    forcedFirstAnswer_.reset();
    const JsonValue* value = forced.response.find("value");
    const JsonValue* chosen = value ? value->find("chosenActionIndex") : nullptr;
    bool matches = who == forced.who && request.at("$case").asString() == "action";
    if (matches) {
      const auto& actions = request.at("value").at("action").asArray();
      const auto& original = forced.actions.asArray();
      matches = actions.size() == original.size();
      if (matches && chosen && chosen->isNumber()) {
        int index = chosen->asInt();
        bool inRange = index >= 0 && static_cast<size_t>(index) < actions.size();
        // JS compares two undefined action entries as equal, then the engine validates the answer.
        matches = !inRange || actions[index].at("action") == original[index].at("action");
      }
    }
    if (matches) return forced.response.at("value");
    JsonValue failed = JsonValue::object();
    failed["type"] = "failed";
    failed["game"] = gameId_;
    failed["message"] = "copy diverged: its first decision differs from the original";
    emit_(failed);
    stopped_ = true;
    game_->terminate();
    throw GameTerminated();
  }
  bool isAction = request.at("$case").asString() == "action";
  if (isAction) ++engineWork.decisions;
  int choices = 0;
  if (isAction) for (const auto& action : pendingActions_[who]) choices += action.validity == 0;
  const auto& view = replyViews_[who];
  bool wanted = replies_ && !view.isNull() && choices > 1 && stepStartState_ && stepStartState_->phase == Phase::Action;
  JsonValue replies, roundEnd;
  {
    NotificationsPaused searchesOnCopies;  // replies and the end of round play copies, not the real game
    if (wanted) {
      const auto* onDemand = view.find("onDemand");
      if (onDemand && onDemand->asBool()) {
        replyContext_[who] = ReplyContext{stepStartState_, pendingActions_[who], view, replySeed_++};
      } else {
        replies = opponentReplies(stepStartState_, who, pendingActions_[who], replySeed_++, view, playedCards_[1 - who]);
      }
    }
    double roundEndSeed = replySeed_++;
    if (wanted) {
      auto cards = opponentCardsFor(stepStartState_, who, roundEndSeed, view, playedCards_[1 - who]);
      roundEnd = endOfRound(stepStartState_, who, roundEndSeed, &cards);
    }
  }
  pending_[who] = Pending{scheduler_.current(), std::move(request), std::nullopt, std::move(replies), std::move(roundEnd)};
  flushHeldNotification();
  queueDecision_(*this, who);
  while (!pending_[who]->response) scheduler_.wait();
  JsonValue response = std::move(*pending_[who]->response);
  pending_[who].reset();
  const JsonValue* value = response.find("value");
  if (!value || !value->isObject()) throw GameIoError(who, "Invalid response");
  return *value;
}

JsonValue GameSession::decisionMessage(int who) const {
  const GameStatePointer& state = game_->state();
  JsonValue view = exposeState(who, state);
  JsonValue canPlunging = JsonValue::array();
  for (const PlayerPointer& player : state->players) canPlunging.push(player->canPlunging);
  view["canPlunging"] = std::move(canPlunging);
  if (privileged_) {
    const auto& opponent = *state->players[1 - who];
    JsonValue hidden = JsonValue::object(), hand = JsonValue::array(), dice = JsonValue::array();
    for (const auto& card : opponent.hands) hand.push(card->definition->id);
    for (int die : opponent.dice) dice.push(die);
    hidden["hand"] = std::move(hand); hidden["dice"] = std::move(dice); view["opponentPrivate"] = std::move(hidden);
  }
  JsonValue message = JsonValue::object();
  message["type"] = "decision";
  message["game"] = gameId_;
  message["who"] = who;
  message["request"] = pending_[who]->request;
  message["state"] = std::move(view);
  message["counters"] = boardCountersById(state);
  if (previews_) message["previews"] = true;
  if (!pending_[who]->replies.isNull()) message["replies"] = pending_[who]->replies;
  if (!pending_[who]->endOfRound.isNull()) message["endOfRound"] = pending_[who]->endOfRound;
  if (replyContext_[who]) message["repliesOnDemand"] = true;
  return message;
}

bool GameSession::respond(int who, const JsonValue& response) {
  if (!pending_[who] || pending_[who]->response) return false;
  if (response.at("$case").asString() == "action") {
    int index = response.at("value").at("chosenActionIndex").asInt();
    if (index >= 0 && static_cast<size_t>(index) < pendingActions_[who].size()) {
      const auto& action = pendingActions_[who][index];
      if (action.type == ActionType::PlayCard) playedCards_[who].push_back(action.skill->caller.entity()->definition->id);
    }
  }
  replyContext_[who].reset();
  pending_[who]->response = response;
  scheduler_.wake(pending_[who]->waiter);
  return true;
}

void GameSession::stop() {
  stopped_ = true;
  if (notifications_) notifications_->stop();
  game_->terminate();
  for (int who : {0, 1}) {
    if (pending_[who] && pending_[who]->waiter) scheduler_.cancel(pending_[who]->waiter);
  }
}

ActionAnswer GameSession::action(int who, const std::vector<ActionInfo>& actions) {
  pendingActions_[who] = actions;
  JsonValue value = askPlayer(who, actionRequest(actions));
  const JsonValue* index = value.find("chosenActionIndex");
  const JsonValue* dice = value.find("usedDice");
  if (!index || !index->isNumber() || index->asNumber() != static_cast<int>(index->asNumber())) {
    throw GameIoError(who, "Invalid response of action: no chosenActionIndex");
  }
  if (!dice || !dice->isArray()) throw GameIoError(who, "Invalid response of action: no usedDice");
  ActionAnswer answer;
  answer.chosenActionIndex = index->asInt();
  for (const JsonValue& die : dice->asArray()) {
    if (!die.isNumber()) throw GameIoError(who, "Invalid response of action: no usedDice");
    answer.usedDice.push_back(die.asInt());
  }
  return answer;
}

int GameSession::chooseActive(int who, const std::vector<int>& candidateIds) {
  JsonValue value = askPlayer(who, chooseActiveRequest(candidateIds));
  const JsonValue* id = value.find("activeCharacterId");
  if (!id || !id->isNumber()) throw GameIoError(who, "Invalid response of chooseActive: no activeCharacterId");
  return id->asInt();
}

std::vector<DiceType> GameSession::rerollDice(int who) {
  JsonValue value = askPlayer(who, rerollDiceRequest());
  const JsonValue* dice = value.find("diceToReroll");
  if (!dice || !dice->isArray()) throw GameIoError(who, "Invalid response of rerollDice: no diceToReroll");
  std::vector<DiceType> result;
  for (const JsonValue& die : dice->asArray()) result.push_back(die.asInt());
  return result;
}

std::vector<int> GameSession::switchHands(int who) {
  JsonValue value = askPlayer(who, switchHandsRequest());
  const JsonValue* ids = value.find("removedHandIds");
  if (!ids || !ids->isArray()) throw GameIoError(who, "Invalid response of switchHands: no removedHandIds");
  std::vector<int> result;
  for (const JsonValue& id : ids->asArray()) result.push_back(id.asInt());
  return result;
}

int GameSession::selectCard(int who, const std::vector<int>& candidateDefinitionIds) {
  JsonValue value = askPlayer(who, selectCardRequest(candidateDefinitionIds));
  const JsonValue* id = value.find("selectedDefinitionId");
  if (!id || !id->isNumber()) throw GameIoError(who, "Invalid response of selectCard: no selectedDefinitionId");
  return id->asInt();
}

void GameSession::all(const std::function<void()>& first, const std::function<void()>& second) {
  // Shared with both children: after a first failure the parent goes on, and the other child may finish later.
  struct Join {
    Scheduler::Task* parent;
    int remaining = 2;
    std::exception_ptr firstError;
  };
  auto join = std::make_shared<Join>(Join{scheduler_.current(), 2, {}});
  auto onFinish = [this, join](Scheduler::Task& task) {
    --join->remaining;
    bool failed = static_cast<bool>(task.fiber->error());
    if (failed && !join->firstError) join->firstError = task.fiber->error();
    // Promise.all settles on the first rejection, or when both are done.
    if (join->parent && (join->remaining == 0 || failed)) scheduler_.wake(join->parent);
  };
  scheduler_.spawn(first, onFinish);
  scheduler_.spawn(second, onFinish);
  while (join->remaining > 0 && !join->firstError) scheduler_.wait();
  join->parent = nullptr;
  if (join->firstError) std::rethrow_exception(join->firstError);
}

}  // namespace gitcg
