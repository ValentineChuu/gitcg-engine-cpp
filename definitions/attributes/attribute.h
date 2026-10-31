// The source-ordered GTS attribute tree; positional values are evaluated by the translator.
#pragma once
#include <string>
#include <vector>
#include "core/json/json_value.h"

namespace gitcg {
struct Attribute {
  std::string name;
  std::vector<JsonValue> positionals;
  std::vector<Attribute> attributes;
  const JsonValue& argument(size_t index) const;
  static Attribute fromJson(const JsonValue& value);
};
JsonValue attributeOptions(const std::vector<Attribute>& attributes);
bool optionBool(const JsonValue& options, std::string_view name, bool fallback);
int lambdaSlot(const JsonValue& value);
}  // namespace gitcg
