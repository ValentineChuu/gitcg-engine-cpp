// Notifications for a seat played by a person (core/src/game.ts notifyOneImpl, mutator.ts notify and resetState,
// io.ts exposeMutation): what Piovium's web board receives, including the mutations that drive its animations.
//
// Piovium's mutator keeps every primitive state mutation since the last notification and sends them, exposed for the
// receiving player, together with the "exposed mutations" named at the notify call (damage, skillUsed, ...). A skill
// runs on its own mutator whose notifications only accumulate: when the skill ends, its state mutations and then its
// exposed mutations reach the outer level as one notification, or are dropped if the skill left the state unchanged.
// Levels here mirror that: the game at the bottom sends, each running skill above it accumulates.
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
  using Emit = std::function<void(JsonValue notification)>;
  NotificationSink(int seat, Emit emit) : seat_(seat), emit_(std::move(emit)) {}

  int seat() const { return seat_; }

  // A primitive state mutation (one `mutate` in the core). `exposed` is its exposeMutation for this seat as
  // {"$case", "value"}, or nullopt when it isn't exposed; it still counts as a mutation to notify.
  void stateMutation(std::optional<JsonValue> exposed);
  // mutator.notify: at the game level sends the pending mutations followed by `exposed` when there is anything
  // (or `force`); inside a skill only accumulates.
  void notify(const GameStatePointer& state, std::vector<JsonValue> exposed = {}, bool force = false);
  // game.ts notifyOne for `who`: the state and `exposed` only, sent even when empty; pending mutations stay.
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
    std::vector<std::optional<JsonValue>> stateMutations;
    std::vector<JsonValue> exposedMutations;
  };
  void send(const GameStatePointer& state, const std::vector<std::optional<JsonValue>>& stateMutations,
            const std::vector<JsonValue>& exposed);

  int seat_;
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
