// Slot failures are explicit; an unbound card action never silently becomes a no-op.
#include "definitions/attributes/lambda_slots.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "core/json/json_text.h"
#include <stdexcept>
#include "core/skills/skill_context.h"

namespace gitcg {
long emittedMessageCount = 0;
void LambdaSlots::bind(int slot, LambdaFunction function) { functions_[slot] = std::move(function); }
JsonValue LambdaSlots::call(int slot, LambdaInvocation invocation) const {
  auto found = functions_.find(slot);
  if (found == functions_.end()) throw std::runtime_error("lambda slot " + std::to_string(slot) + " not translated");
  // GITCG_TRACE_SLOTS=1: log each call with the number of messages the worker has emitted (debugging only).
  static const bool trace = std::getenv("GITCG_TRACE_SLOTS") != nullptr;
  if (trace) std::fprintf(stderr, "slot %d after message %ld\n", slot, emittedMessageCount);
  try {
    return found->second(invocation);
  } catch (const std::out_of_range& error) {
    // Translated card code indexing past an array's end (JS would read undefined): name the slot.
    throw std::runtime_error("lambda slot " + std::to_string(slot) + ": " + error.what());
  }
}
JsonValue LambdaSlots::call(int slot, SkillContext& context) const {
  return call(slot, {&context, context.rawState(), context.self().id(), &context.eventArgument(), nullptr});
}
std::string jsString(const JsonValue& value) {
  if (value.isString()) return value.asString();
  if (value.isNumber()) {
    double number = value.asNumber();
    if (std::isnan(number)) return "NaN";
    if (std::isinf(number)) return number < 0 ? "-Infinity" : "Infinity";
    if (number == 0) return "0";
    return writeJson(value);
  }
  if (value.isNull()) return "null";
  if (value.isBool()) return value.asBool() ? "true" : "false";
  throw std::runtime_error("Expected a string or number description result");
}
bool jsTruthy(const JsonValue& value) {
  if (value.isNull()) return false;
  if (value.isBool()) return value.asBool();
  if (value.isNumber()) return value.asNumber() != 0 && !std::isnan(value.asNumber());
  if (value.isString()) return !value.asString().empty();
  return true;
}
}  // namespace gitcg
