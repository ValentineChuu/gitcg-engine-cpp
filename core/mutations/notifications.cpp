#include "core/mutations/notifications.h"

#include <algorithm>
#include <stdexcept>

#include "core/expose/expose_state.h"

namespace gitcg {

thread_local NotificationSink* notificationSink = nullptr;

JsonValue exposedMutation(const char* kind, JsonValue value) {
  JsonValue mutation = JsonValue::object();
  mutation["$case"] = kind;
  mutation["value"] = std::move(value);
  return mutation;
}

NotificationSink::NotificationSink(std::vector<int> seats, Emit emit) : seats_(std::move(seats)), emit_(std::move(emit)) {
  std::sort(seats_.begin(), seats_.end());
  seats_.erase(std::unique(seats_.begin(), seats_.end()), seats_.end());
  for (int seat : seats_) {
    if (seat < 0 || seat > 1) throw std::runtime_error("a person's seat must be 0 or 1");
  }
}

void NotificationSink::stateMutation(std::optional<JsonValue> exposed) {
  for (int seat : seats_) top().stateMutations[seat].push_back(exposed);
}

void NotificationSink::stateMutation(const ExposedFor& exposedFor) {
  for (int seat : seats_) top().stateMutations[seat].push_back(exposedFor(seat));
}

void NotificationSink::notify(const GameStatePointer& state, std::vector<JsonValue> exposed, bool force) {
  if (!skills_.empty()) {
    // A skill's own mutator: its notifications accumulate in the skill's saved notification (skill_context.ts onNotify).
    for (JsonValue& mutation : exposed) skills_.back().exposedMutations.push_back(std::move(mutation));
    return;
  }
  for (int seat : seats_) {
    if (force || !game_.stateMutations[seat].empty() || !exposed.empty()) send(seat, state, game_.stateMutations[seat], exposed);
    game_.stateMutations[seat].clear();
  }
}

void NotificationSink::notifyOne(int who, const GameStatePointer& state, std::vector<JsonValue> exposed) {
  if (std::find(seats_.begin(), seats_.end(), who) != seats_.end()) send(who, state, {}, exposed);
}

void NotificationSink::setStatus(int who, PlayerStatus status, const GameStatePointer& state) {
  status_[who] = status;
  JsonValue value = JsonValue::object();
  value["who"] = who;
  value["status"] = static_cast<int>(status);
  std::vector<JsonValue> exposed;
  exposed.push_back(exposedMutation("playerStatusChange", std::move(value)));
  notify(state, std::move(exposed));
}

void NotificationSink::beginSkill() {
  skills_.emplace_back();
}

void NotificationSink::endSkill(const GameStatePointer& state, bool changed, std::vector<JsonValue> prependExposed) {
  Level finished = std::move(skills_.back());
  skills_.pop_back();
  if (!changed) return;
  // resetState: the skill's state mutations become the pending ones, then notify with its exposed mutations.
  std::vector<JsonValue> exposed = std::move(prependExposed);
  for (JsonValue& mutation : finished.exposedMutations) exposed.push_back(std::move(mutation));
  for (int seat : seats_) {
    for (auto& mutation : finished.stateMutations[seat]) top().stateMutations[seat].push_back(std::move(mutation));
  }
  notify(state, std::move(exposed));
}

void NotificationSink::send(int who, const GameStatePointer& state, const std::vector<std::optional<JsonValue>>& stateMutations,
                            const std::vector<JsonValue>& exposed) {
  if (stopped_) return;
  JsonValue mutations = JsonValue::array();
  auto add = [&](const JsonValue& mutation) {
    JsonValue wrapped = JsonValue::object();
    wrapped["mutation"] = mutation;
    mutations.push(std::move(wrapped));
  };
  for (const auto& mutation : stateMutations) {
    if (mutation) add(*mutation);
  }
  for (const JsonValue& mutation : exposed) add(mutation);
  JsonValue exposedState = exposeState(who, state);
  for (int player = 0; player < 2; ++player) exposedState["player"].asArray()[player]["status"] = static_cast<int>(status_[player]);
  JsonValue notification = JsonValue::object();
  notification["mutation"] = std::move(mutations);
  notification["state"] = std::move(exposedState);
  emit_(who, std::move(notification));
}

}  // namespace gitcg
