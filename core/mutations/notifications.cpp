#include "core/mutations/notifications.h"

#include "core/expose/expose_state.h"

namespace gitcg {

thread_local NotificationSink* notificationSink = nullptr;

JsonValue exposedMutation(const char* kind, JsonValue value) {
  JsonValue mutation = JsonValue::object();
  mutation["$case"] = kind;
  mutation["value"] = std::move(value);
  return mutation;
}

void NotificationSink::stateMutation(std::optional<JsonValue> exposed) {
  (skills_.empty() ? game_ : skills_.back()).stateMutations.push_back(std::move(exposed));
}

void NotificationSink::notify(const GameStatePointer& state, std::vector<JsonValue> exposed, bool force) {
  if (!skills_.empty()) {
    // A skill's own mutator: its notifications accumulate in the skill's saved notification (skill_context.ts onNotify).
    for (JsonValue& mutation : exposed) skills_.back().exposedMutations.push_back(std::move(mutation));
    return;
  }
  if (force || !game_.stateMutations.empty() || !exposed.empty()) send(state, game_.stateMutations, exposed);
  game_.stateMutations.clear();
}

void NotificationSink::notifyOne(int who, const GameStatePointer& state, std::vector<JsonValue> exposed) {
  if (who == seat_) send(state, {}, exposed);
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
  Level& below = skills_.empty() ? game_ : skills_.back();
  for (auto& mutation : finished.stateMutations) below.stateMutations.push_back(std::move(mutation));
  notify(state, std::move(exposed));
}

void NotificationSink::send(const GameStatePointer& state, const std::vector<std::optional<JsonValue>>& stateMutations,
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
  JsonValue exposedState = exposeState(seat_, state);
  for (int who = 0; who < 2; ++who) exposedState["player"].asArray()[who]["status"] = static_cast<int>(status_[who]);
  JsonValue notification = JsonValue::object();
  notification["mutation"] = std::move(mutations);
  notification["state"] = std::move(exposedState);
  emit_(std::move(notification));
}

}  // namespace gitcg
