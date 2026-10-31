// An entity's or character's variables: a JS object of numbers, so iteration follows insertion order, and
// setting an existing name keeps its place. Values are doubles, as in JS.
#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "core/values/names.h"

namespace gitcg {

class Variables {
 public:
  std::optional<double> find(Name name) const {
    for (const auto& [key, value] : entries_) {
      if (key == name) return value;
    }
    return std::nullopt;
  }
  // `variables[name]` where a missing name reads as undefined; callers decide what undefined means.
  double valueOr(Name name, double fallback) const { return find(name).value_or(fallback); }
  bool has(Name name) const { return find(name).has_value(); }

  void set(Name name, double value) {
    for (auto& [key, existing] : entries_) {
      if (key == name) {
        existing = value;
        return;
      }
    }
    entries_.emplace_back(name, value);
  }

  const std::vector<std::pair<Name, double>>& entries() const { return entries_; }

 private:
  std::vector<std::pair<Name, double>> entries_;
};

}  // namespace gitcg
