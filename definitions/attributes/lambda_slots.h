// Opaque card-code slots. Bindings are looked up at call time so definitions can be built before translation.
#pragma once
#include <functional>
#include <unordered_map>
#include "core/json/json_value.h"
#include "core/state/game_state.h"

namespace gitcg {
class SkillContext;
struct SkillArgument;
struct LambdaInvocation {
  SkillContext* context = nullptr;
  GameStatePointer state;
  int selfId = 0;
  SkillArgument* argument = nullptr;
  JsonValue* extensionState = nullptr;
};
using LambdaFunction = std::function<JsonValue(LambdaInvocation&)>;
class LambdaSlots {
 public:
  void bind(int slot, LambdaFunction function);
  JsonValue call(int slot, LambdaInvocation invocation) const;
  JsonValue call(int slot, SkillContext& context) const;
 private:
  std::unordered_map<int, LambdaFunction> functions_;
};
bool jsTruthy(const JsonValue& value);
// Messages the worker has sent so far, for slot traces (GITCG_TRACE_SLOTS).
extern long emittedMessageCount;
std::string jsString(const JsonValue& value);
}  // namespace gitcg
