// One protocol game (reference/bridge/game_session.mjs): player requests suspend a fiber; copies restart from the
// saved step-start state. Optional previews and searches enrich decisions without advancing the real game.
#pragma once

#include <functional>
#include <memory>
#include <optional>

#include "bridge/fibers/scheduler.h"
#include "bridge/session/combat_log.h"
#include "core/game/game.h"
#include "core/json/json_value.h"
#include "core/mutations/notifications.h"

namespace gitcg {

class GameSession : private PlayerIo, private Concurrency {
 public:
  using Emit = std::function<void(const JsonValue& message)>;
  // `queueDecision` is called when a decision is waiting; the worker emits queued decisions once no fiber can run.
  using QueueDecision = std::function<void(GameSession& session, int who)>;
  struct ForcedAnswer {
    int who;
    JsonValue actions;
    JsonValue response;
  };

  GameSession(int gameId, GameStatePointer initialState, Scheduler& scheduler, Emit emit, QueueDecision queueDecision,
              std::optional<ForcedAnswer> forcedFirstAnswer = std::nullopt);
  ~GameSession();

  void start();
  // Entities placed in the initial state before the game (a stage's starting equipment, initial_equipment.mjs): the
  // core creates them with pending mutations, so a person's first notification shows them.
  void recordInitialEntities(const std::vector<std::pair<EntityArea, int>>& created);
  // humanSeats: the seats people play (one against the bot, both between two people); each gets its board's
  // notifications. compactNotifications (with humanSeats): notifications that only mark events opening and closing
  // are merged into the next one (see sendNotification), for a browser board: the same mutations in order, about a
  // quarter as many messages. Not part of the reference protocol.
  void configure(bool previews, bool replies, const JsonValue* replyViews, double seed, bool privileged,
                 std::vector<int> humanSeats = {}, bool compactNotifications = false);
  void computeReplies(int who, const JsonValue& indices);
  std::unique_ptr<GameSession> copy(int copyId, int who, const JsonValue& response, double seed,
                                    const JsonValue* opponentCards = nullptr);
  // Answers a pending decision; false when there is none for that player.
  bool respond(int who, const JsonValue& response);
  void stop();
  bool done() const { return done_; }
  int gameId() const { return gameId_; }

  // The decision message for `who`'s pending request, built from the current state (called by the worker).
  JsonValue decisionMessage(int who) const;

 private:
  struct Pending {
    Scheduler::Task* waiter = nullptr;
    JsonValue request;
    std::optional<JsonValue> response;
    JsonValue replies;
    JsonValue endOfRound;
  };

  JsonValue askPlayer(int who, JsonValue request);  // the response's value
  ActionAnswer action(int who, const std::vector<ActionInfo>& actions) override;
  int chooseActive(int who, const std::vector<int>& candidateIds) override;
  std::vector<DiceType> rerollDice(int who) override;
  std::vector<int> switchHands(int who) override;
  int selectCard(int who, const std::vector<int>& candidateDefinitionIds) override;
  void all(const std::function<void()>& first, const std::function<void()>& second) override;
  void run();
  void finish(JsonValue message);

  int gameId_;
  Scheduler& scheduler_;
  Emit emit_;
  QueueDecision queueDecision_;
  std::unique_ptr<Game> game_;
  CombatLog combatLog_;
  GameStatePointer stepStartState_;
  std::optional<ForcedAnswer> forcedFirstAnswer_;
  struct ReplyContext {
    GameStatePointer state;
    std::vector<ActionInfo> actions;
    JsonValue view;
    double seed;
  };
  bool previews_ = false;
  bool replies_ = false;
  bool privileged_ = false;
  std::unique_ptr<NotificationSink> notifications_;  // with a person's seat: their boards' notifications
  bool compactNotifications_ = false;
  std::optional<JsonValue> heldNotification_[2];  // compact mode, per seat: the latest held notification, with every held mutation
  void sendNotification(int who, JsonValue notification);
  void flushHeldNotifications();
  JsonValue replyViews_[2];
  double replySeed_ = 0;
  std::vector<int> playedCards_[2];
  std::vector<ActionInfo> pendingActions_[2];
  std::optional<ReplyContext> replyContext_[2];
  std::optional<Pending> pending_[2];
  bool stopped_ = false;
  bool done_ = false;
};

}  // namespace gitcg
