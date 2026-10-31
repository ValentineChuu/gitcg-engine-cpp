// GTS variable bounds, append recreation and usage declarations, with JS visibility and duplicate rules.
#include "definitions/entities/entity_model.h"
#include <stdexcept>
#include "core/values/usage_per_round_names.h"

namespace gitcg {
VariableConfig createVariableConfig(double initial, const JsonValue& options) {
  VariableConfig result;
  result.initialValue = initial;
  if (optionBool(options, "forceOverwrite", false)) result.recreateBehavior = RecreateBehavior::Overwrite;
  if (const auto* range = options.find("range")) {
    if (range->isNumber()) { result.lowerBound = 0; result.upperBound = range->asNumber(); }
    else { result.lowerBound = range->asArray().at(0).asNumber(); result.upperBound = range->asArray().at(1).asNumber(); }
  }
  if (result.lowerBound > result.upperBound) throw std::runtime_error("Variable range must have min <= max");
  if (const auto* append = options.find("append"); append && !(append->isBool() && !append->asBool())) {
    result.recreateBehavior = RecreateBehavior::Append;
    result.appendLimit = result.upperBound;
    result.appendValue = initial;
    if (append->isNumber()) result.appendLimit = append->asNumber();
    else if (append->isObject()) {
      if (auto* value = append->find("limit")) result.appendLimit = value->asNumber();
      if (auto* value = append->find("value")) result.appendValue = value->asNumber();
    }
    if (result.appendLimit > result.upperBound) throw std::runtime_error("Variable range upper bound must be >= append limit");
  }
  return result;
}
void EntityModel::setVariable(std::string name, double value, const JsonValue& options) {
  Name key = intern(name);
  if (definition.findVarConfig(key)) throw std::runtime_error("Variable " + name + " already exists in " + std::to_string(id));
  definition.varConfigs.emplace_back(key, createVariableConfig(value, options));
  if (optionBool(options, "visible", true)) definition.visibleVarName = key;
}
Name EntityModel::setUsage(double count, JsonValue usageOptions, bool perRound) {
  std::string name;
  if (auto* value = usageOptions.find("name")) name = value->asString();
  else if (owner == OwnerType::Character) throw std::runtime_error("Passive skill usage requires an explicit name");
  else if (perRound) {
    if (usagePerRoundIndex >= 16) throw std::runtime_error("Too many usagePerRound variables");
    name = usagePerRoundIndex == 0 ? "usagePerRound" : "usagePerRound" + std::to_string(usagePerRoundIndex);
    ++usagePerRoundIndex;
  } else name = "usage";
  Name key = intern(name);
  bool autoDispose = name == "usage" && optionBool(usageOptions, "autoDispose", true);
  if (autoDispose) {
    if (owner == OwnerType::Character || owner == OwnerType::Attachment) throw std::runtime_error("This entity cannot autoDispose usage");
    definition.disposeWhenUsageIsZero = true;
  }
  JsonValue declaration = JsonValue::object();
  declaration["perRound"] = perRound;
  declaration["autoDispose"] = autoDispose;
  declaration["autoDecrease"] = optionBool(usageOptions, "autoDecrease", true);
  // Visibility is deliberately not updated when a compatible declaration is ignored.
  if (const auto* existing = definition.findVarConfig(key)) {
    auto incoming = createVariableConfig(count, usageOptions);
    bool same = existing->initialValue == incoming.initialValue && existing->lowerBound == incoming.lowerBound &&
      existing->upperBound == incoming.upperBound && existing->recreateBehavior == incoming.recreateBehavior &&
      existing->appendLimit == incoming.appendLimit && existing->appendValue == incoming.appendValue;
    for (const auto& [declaredName, info] : declaredUsages) if (declaredName == key && same && info == declaration) return key;
    throw std::runtime_error("Incompatible usage " + name + " in " + std::to_string(id));
  }
  setVariable(name, count, usageOptions);
  declaredUsages.emplace_back(key, declaration);
  return key;
}
}  // namespace gitcg
