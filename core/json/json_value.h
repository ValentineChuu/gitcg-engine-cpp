// A JSON-like value: protocol messages, and extension states (arbitrary JS data in the core).
// Objects keep insertion order like JS objects.
#pragma once

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace gitcg {

class JsonValue {
 public:
  using Array = std::vector<JsonValue>;
  using Object = std::vector<std::pair<std::string, JsonValue>>;

  JsonValue() : value_(nullptr) {}
  JsonValue(std::nullptr_t) : value_(nullptr) {}
  JsonValue(bool value) : value_(value) {}
  JsonValue(int value) : value_(static_cast<double>(value)) {}
  JsonValue(int64_t value) : value_(static_cast<double>(value)) {}
  JsonValue(double value) : value_(value) {}
  JsonValue(const char* value) : value_(std::string(value)) {}
  JsonValue(std::string value) : value_(std::move(value)) {}
  JsonValue(Array value) : value_(std::move(value)) {}
  JsonValue(Object value) : value_(std::move(value)) {}

  static JsonValue array() { return JsonValue(Array{}); }
  static JsonValue object() { return JsonValue(Object{}); }

  bool isNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
  bool isBool() const { return std::holds_alternative<bool>(value_); }
  bool isNumber() const { return std::holds_alternative<double>(value_); }
  bool isString() const { return std::holds_alternative<std::string>(value_); }
  bool isArray() const { return std::holds_alternative<Array>(value_); }
  bool isObject() const { return std::holds_alternative<Object>(value_); }

  bool asBool() const { return std::get<bool>(value_); }
  double asNumber() const { return std::get<double>(value_); }
  int asInt() const { return static_cast<int>(std::get<double>(value_)); }
  const std::string& asString() const { return std::get<std::string>(value_); }
  const Array& asArray() const { return std::get<Array>(value_); }
  Array& asArray() { return std::get<Array>(value_); }
  const Object& asObject() const { return std::get<Object>(value_); }
  Object& asObject() { return std::get<Object>(value_); }

  // Object field access; `find` returns null when missing.
  const JsonValue* find(std::string_view key) const;
  JsonValue& operator[](std::string_view key);  // inserts null when missing (objects only)
  const JsonValue& at(std::string_view key) const;
  void push(JsonValue value) { asArray().push_back(std::move(value)); }

  bool operator==(const JsonValue& other) const { return value_ == other.value_; }

 private:
  std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value_;
};

}  // namespace gitcg
