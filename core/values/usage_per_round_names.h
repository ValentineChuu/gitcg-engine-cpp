// The variable names of per-round usages (core/src/base/entity.ts USAGE_PER_ROUND_VARIABLE_NAMES):
// usagePerRound, usagePerRound1 ... usagePerRound15.
#pragma once

#include <string>
#include <vector>

#include "core/values/names.h"

namespace gitcg {

inline const std::vector<Name>& usagePerRoundNames() {
  static const std::vector<Name> names = [] {
    std::vector<Name> result{intern("usagePerRound")};
    for (int index = 1; index <= 15; ++index) result.push_back(intern("usagePerRound" + std::to_string(index)));
    return result;
  }();
  return names;
}

inline bool isUsagePerRoundName(Name name) {
  for (Name candidate : usagePerRoundNames()) {
    if (candidate == name) return true;
  }
  return false;
}

}  // namespace gitcg
