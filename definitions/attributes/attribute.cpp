// Parsing and simple view-model options, including boolean switches and recursive option objects.
#include "definitions/attributes/attribute.h"
#include <stdexcept>

namespace gitcg {
const JsonValue& Attribute::argument(size_t index) const {
  static const JsonValue missing;
  return index < positionals.size() ? positionals[index] : missing;
}
Attribute Attribute::fromJson(const JsonValue& value) {
  Attribute result;
  result.name = value.at("name").asString();
  result.positionals = value.at("positionals").asArray();
  for (const auto& child : value.at("attributes").asArray()) result.attributes.push_back(fromJson(child));
  return result;
}
JsonValue attributeOptions(const std::vector<Attribute>& attributes) {
  auto result = JsonValue::object();
  for (const auto& attribute : attributes) {
    if (!attribute.attributes.empty()) result[attribute.name] = attributeOptions(attribute.attributes);
    else result[attribute.name] = attribute.positionals.empty() ? JsonValue(true) : attribute.positionals.front();
  }
  return result;
}
bool optionBool(const JsonValue& options, std::string_view name, bool fallback) {
  auto value = options.find(name);
  return value ? value->asBool() : fallback;
}
int lambdaSlot(const JsonValue& value) {
  auto slot = value.isObject() ? value.find("$lambda") : nullptr;
  return slot ? slot->asInt() : 0;
}
}  // namespace gitcg
