// A list of emitted events (core/src/mutator.ts EventList): a damage event to a target already damaged in this
// list is merged into the first one (values summed, causeDefeated and fromReaction or-ed, first event's state kept).
#pragma once

#include <unordered_map>
#include <vector>

#include "core/events/event_arg.h"

namespace gitcg {

class EventList {
 public:
  void push(EventAndArgument event);
  void append(const std::vector<EventAndArgument>& events) {
    for (const EventAndArgument& event : events) push(event);
  }
  void append(const EventList& events) { append(events.items()); }

  const std::vector<EventAndArgument>& items() const { return items_; }
  std::vector<EventAndArgument>& items() { return items_; }
  bool empty() const { return items_.empty(); }

 private:
  std::vector<EventAndArgument> items_;
  std::unordered_map<int, size_t> damageIndexByTarget_;
};

}  // namespace gitcg
