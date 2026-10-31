#include "core/json/json_value.h"

#include <stdexcept>

namespace gitcg {

const JsonValue* JsonValue::find(std::string_view key) const {
  if (!isObject()) return nullptr;
  for (const auto& [name, value] : asObject()) {
    if (name == key) return &value;
  }
  return nullptr;
}

JsonValue& JsonValue::operator[](std::string_view key) {
  Object& fields = asObject();
  for (auto& [name, value] : fields) {
    if (name == key) return value;
  }
  fields.emplace_back(std::string(key), JsonValue());
  return fields.back().second;
}

const JsonValue& JsonValue::at(std::string_view key) const {
  const JsonValue* value = find(key);
  if (!value) throw std::out_of_range("missing JSON field " + std::string(key));
  return *value;
}

}  // namespace gitcg
