// A dice cost: a JS Map from dice type to count. Iteration follows insertion order, which the exposed
// `requiredCost` and the cost rules (appendCost, normalizeCost) depend on.
#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "core/values/game_enums.h"

namespace gitcg {

class DiceRequirement {
 public:
  std::optional<int> find(DiceType type) const {
    for (const auto& [key, count] : entries_) {
      if (key == type) return count;
    }
    return std::nullopt;
  }
  int countOf(DiceType type) const { return find(type).value_or(0); }
  bool has(DiceType type) const { return find(type).has_value(); }
  size_t size() const { return entries_.size(); }

  void set(DiceType type, int count) {
    for (auto& [key, existing] : entries_) {
      if (key == type) {
        existing = count;
        return;
      }
    }
    entries_.emplace_back(type, count);
  }
  void erase(DiceType type) {
    for (auto entry = entries_.begin(); entry != entries_.end(); ++entry) {
      if (entry->first == type) {
        entries_.erase(entry);
        return;
      }
    }
  }
  void clear() { entries_.clear(); }

  const std::vector<std::pair<DiceType, int>>& entries() const { return entries_; }

 private:
  std::vector<std::pair<DiceType, int>> entries_;
};

}  // namespace gitcg
