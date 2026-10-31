// Optional mutation observation for previews. Ordinary games pay only a null pointer check. Observation is scoped
// to a synchronous preview; no observer is kept in persistent state or across a suspended protocol decision.
#pragma once

#include <functional>
#include "core/events/damage_info.h"
#include "core/events/entity_event_args.h"

namespace gitcg {
struct SwitchActiveInfo;

using MutationObserver = std::function<void(JsonValue)>;
extern thread_local MutationObserver* mutationObserver;
// resetState discards a skill's inner notifications if immer returned its original root. Buffer the same records
// until the native skill outcome tells us whether it changed state.
class ObservedSkill {
 public:
  ObservedSkill();
  ~ObservedSkill();
  void finish(bool changed);
 private:
  MutationObserver* previous_;
  MutationObserver observer_;
  std::vector<JsonValue> records_;
  bool finished_ = false;
};
void observeMutation(const char* kind, JsonValue value);
// The exposed mutations of a damage or heal, an aura applied without damage, and a switch ({"$case", "value"}),
// shared by preview observation and notifications (core/mutations/notifications.h).
JsonValue damageMutation(const DamageInfo& info, Aura newAura, double oldHealth, double newHealth, Reaction reaction);
JsonValue auraMutation(int element, const CharacterPointer& target, Aura newAura, Reaction reaction);
JsonValue switchMutation(const SwitchActiveInfo& info);
void observeDamage(const DamageInfo& info, Aura newAura, double oldHealth, double newHealth, Reaction reaction);
void observeAura(int element, const CharacterPointer& target, Aura newAura, Reaction reaction);
void observeSwitch(const SwitchActiveInfo& info);
int exposedArea(AreaType type);
int exposedRemoveReason(RemoveReason reason);
int exposedMoveReason(MoveReason reason);

}  // namespace gitcg
