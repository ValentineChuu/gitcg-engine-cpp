// Notifications for a seat played by a person (core/src/game.ts notifyOneImpl, mutator.ts notify and resetState,
// io.ts exposeMutation): what Piovium's web board receives, including the mutations that drive its animations.
//
// Piovium's mutator keeps every primitive state mutation since the last notification and sends them, exposed for the
// receiving player, together with the "exposed mutations" named at the notify call (damage, skillUsed, ...). A skill
// runs on its own mutator whose notifications only accumulate: when the skill ends, its state mutations and then its
// exposed mutations reach the outer level as one notification, or are dropped if the skill left the state unchanged.
// Levels here mirror that: the game at the bottom sends, each running skill above it accumulates.
//
// One sink serves every seat a person plays (one in a game against the bot, both in a game between two people): each
// seat keeps its own pending mutations, exposed for that seat, and receives its own notifications.
//
// Games without a person's seat have no sink: recording costs them a null check. The worker runs many games on one
// thread, so the scheduler points `notificationSink` at the running fiber's sink on every switch.
#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "core/json/json_value.h"
#include "core/state/game_state.h"

namespace gitcg {

// PbPlayerStatus, set while the game waits for a player (game.ts setPlayerStatus).
enum class PlayerStatus { Unspecified = 0, ChoosingActive = 1, SwitchingHands = 2, Rerolling = 3, Acting = 4, SelectingCards = 5 };

class NotificationSink {
 public:
  using Emit = std::function<void(int who, JsonValue notification)>;
  using ExposedFor = std::function<std::optional<JsonValue>(int seat)>;
  NotificationSink(std::vector<int> seats, Emit emit);

  // A primitive state mutation (one `mutate` in the core). `exposed` is its exposeMutation as {"$case", "value"},
  // the same for every seat, or nullopt when it isn't exposed; it still counts as a mutation to notify.
  void stateMutation(std::optional<JsonValue> exposed);
  // A state mutation exposed differently per seat (hidden cards, the opponent's dice).
  void stateMutation(const ExposedFor& exposedFor);
  // mutator.notify: at the game level sends each seat its pending mutations followed by `exposed` when there is
  // anything (or `force`); inside a skill only accumulates.
  void notify(const GameStatePointer& state, std::vector<JsonValue> exposed = {}, bool force = false);
  // game.ts notifyOne for `who` (when a person plays it): the state and `exposed` only, sent even when empty;
  // pending mutations stay.
  void notifyOne(int who, const GameStatePointer& state, std::vector<JsonValue> exposed = {});
  // game.ts setPlayerStatus: records the status shown in every later notification and notifies the change.
  void setStatus(int who, PlayerStatus status, const GameStatePointer& state);

  // A skill starts running on its own mutator; endSkill merges what it notified into the level below (changed) or
  // drops it (Piovium's resetState returns early when the state is the same object).
  void beginSkill();
  void endSkill(const GameStatePointer& state, bool changed, std::vector<JsonValue> prependExposed = {});
  // A stopped game sends nothing more (game.ts drops notifications once terminated), also while its fiber unwinds.
  void stop() { stopped_ = true; }

 private:
  struct Level {
    std::vector<std::optional<JsonValue>> stateMutations[2];  // per seat a person plays
    std::vector<JsonValue> exposedMutations;
  };
  void send(int who, const GameStatePointer& state, const std::vector<std::optional<JsonValue>>& stateMutations,
            const std::vector<JsonValue>& exposed);
  Level& top() { return skills_.empty() ? game_ : skills_.back(); }

  std::vector<int> seats_;
  Emit emit_;
  Level game_;                // the mutator's pending state mutations (its exposedMutations stay empty)
  std::vector<Level> skills_; // running skills, innermost last
  PlayerStatus status_[2] = {PlayerStatus::Unspecified, PlayerStatus::Unspecified};
  bool stopped_ = false;
};

// The sink of the game running on this thread now, or null.
extern thread_local NotificationSink* notificationSink;

// Pauses notifications for a computation that isn't the real game (previews, look-ahead on copies).
class NotificationsPaused {
 public:
  NotificationsPaused() : saved_(notificationSink) { notificationSink = nullptr; }
  ~NotificationsPaused() { notificationSink = saved_; }
  NotificationsPaused(const NotificationsPaused&) = delete;
  NotificationsPaused& operator=(const NotificationsPaused&) = delete;
 private:
  NotificationSink* saved_;
};

// {"$case": kind, "value": value}, the shape of one exposed mutation.
JsonValue exposedMutation(const char* kind, JsonValue value);

}  // namespace gitcg
