#include "core/events/event_list.h"

#include "core/events/damage_event_args.h"

namespace gitcg {

void EventList::push(EventAndArgument event) {
  if (event.name == EventName::OnDamageOrHeal) {
    auto& damageEvent = static_cast<DamageOrHealEventArg&>(*event.argument);
    if (damageEvent.isDamageTypeDamage()) {
      int targetId = damageEvent.target()->id;
      auto previous = damageIndexByTarget_.find(targetId);
      if (previous != damageIndexByTarget_.end()) {
        auto& previousEvent = static_cast<DamageOrHealEventArg&>(*items_[previous->second].argument);
        DamageInfo combined = previousEvent.damageInfo();
        DamageInfo incoming = damageEvent.damageInfo();
        combined.value += incoming.value;
        combined.causeDefeated = combined.causeDefeated || incoming.causeDefeated;
        if (combined.fromReaction == reaction::None) combined.fromReaction = incoming.fromReaction;
        items_[previous->second].argument =
            std::make_shared<DamageOrHealEventArg>(previousEvent.onTimeState, combined, previousEvent.option);
        return;
      }
      damageIndexByTarget_.emplace(targetId, items_.size());
    }
  }
  items_.push_back(std::move(event));
}

}  // namespace gitcg
