// Query S-expressions and native equivalents of JavaScript function specifications (AGPL-3.0).
#pragma once

#include <functional>
#include <bit>
#include <cstdint>
#include <map>
#include <string>

#include "core/json/json_value.h"

namespace gitcg {

using QueryVariables = std::function<double(const std::string&)>;
using QueryFunction = std::function<double(const QueryVariables&)>;

struct QueryExpression {
  JsonValue expression;
  std::map<std::string, QueryFunction> functions;

  QueryExpression() = default;
  QueryExpression(JsonValue value) : expression(std::move(value)) {}
};

namespace query {
// Callbacks see missing properties as NaN; the JS-function adapter also needs its undefined identity.
inline double undefinedVariable() { return std::bit_cast<double>(uint64_t{0x7ff8000000000001}); }
inline JsonValue list(std::initializer_list<JsonValue> values) { return JsonValue::Array(values); }
// These names are variable keys only in the C++ callback API; JSON uses ["special:..."] nodes.
inline constexpr const char* diceCostKey = "@gitcg.diceCost";
inline constexpr const char* inInitialPileKey = "@gitcg.inInitialPile";
inline JsonValue variableExpression(JsonValue value) {
  if (value.isString() && value.asString() == diceCostKey) return list({"special:diceCost"});
  if (value.isString() && value.asString() == inInitialPileKey) return list({"special:inInitialPile"});
  return value;
}
}  // namespace query
}  // namespace gitcg
