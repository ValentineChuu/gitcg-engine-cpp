// How one variable of an entity or character is created and re-created (core/src/base/entity.ts VariableConfig).
#pragma once

#include <algorithm>

namespace gitcg {

inline constexpr double DEFAULT_VARIABLE_UPPER_BOUND = 2147483647.0;
inline constexpr double DEFAULT_VARIABLE_LOWER_BOUND = -2147483648.0;

enum class RecreateBehavior { Default, Overwrite, Keep, TakeMax, Append };

struct VariableConfig {
  double initialValue = 0;
  double lowerBound = DEFAULT_VARIABLE_LOWER_BOUND;
  double upperBound = DEFAULT_VARIABLE_UPPER_BOUND;
  RecreateBehavior recreateBehavior = RecreateBehavior::Default;
  double appendLimit = 0;  // Append only
  double appendValue = 0;  // Append only
};

inline double clampVariable(double value, const VariableConfig& config) {
  return std::min(config.upperBound, std::max(config.lowerBound, value));
}

}  // namespace gitcg
