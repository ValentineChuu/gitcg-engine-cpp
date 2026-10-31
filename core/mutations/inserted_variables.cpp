#include "core/mutations/inserted_variables.h"

#include <algorithm>

namespace gitcg {

namespace {

std::optional<double> findIncoming(const std::vector<std::pair<Name, double>>& incoming, Name name) {
  for (const auto& [key, value] : incoming) {
    if (key == name) return value;
  }
  return std::nullopt;
}

}  // namespace

EntityPointer findOverriddenEntity(const std::vector<AnyState>& existing, const EntityDefinition& incoming) {
  if (incoming.type == EntityType::Support) return nullptr;
  for (const AnyState& state : existing) {
    if (state.isEntity() && state.entity()->definition->id == incoming.id) return state.entity();
  }
  return nullptr;
}

std::vector<std::pair<Name, double>> insertedStateVariables(const GameState& state, const EntityPointer& oldState,
                                                            const EntityDefinition& definition,
                                                            const std::vector<std::pair<Name, double>>& incomingVariables,
                                                            bool modifyOverriddenVariablesOnly) {
  std::vector<std::pair<Name, double>> result;
  if (!oldState) {
    for (const auto& [name, config] : definition.varConfigs) {
      result.emplace_back(name, clampVariable(findIncoming(incomingVariables, name).value_or(config.initialValue), config));
    }
    return result;
  }
  for (const auto& [name, config] : definition.varConfigs) {
    std::optional<double> incoming = findIncoming(incomingVariables, name);
    if (modifyOverriddenVariablesOnly && !incoming) continue;
    double initialValue = incoming.value_or(config.initialValue);
    double oldValue = oldState->variables.valueOr(name, 0);
    switch (config.recreateBehavior) {
      case RecreateBehavior::Default:
        result.emplace_back(name, state.versionBehavior.recreateTakesMax ? std::max(initialValue, oldValue) : initialValue);
        break;
      case RecreateBehavior::Keep: break;
      case RecreateBehavior::Overwrite: result.emplace_back(name, initialValue); break;
      case RecreateBehavior::TakeMax: result.emplace_back(name, std::max(initialValue, oldValue)); break;
      case RecreateBehavior::Append: {
        if (oldValue > config.appendLimit) break;
        double appendValue = incoming.value_or(config.appendValue);
        result.emplace_back(name, std::min(appendValue + oldValue, config.appendLimit));
        break;
      }
    }
  }
  for (auto& [name, value] : result) value = clampVariable(value, *definition.findVarConfig(name));
  return result;
}

}  // namespace gitcg
