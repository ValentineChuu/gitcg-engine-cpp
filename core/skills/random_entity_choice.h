// JS random([]) returns undefined after advancing the generator. A handle choice must retain that absence until
// translated code tests it; slicing a present choice back to EntityHandle still gives the usual live view.
#pragma once
#include "core/skills/state_handles.h"
#include "core/values/engine_errors.h"

namespace gitcg {
class RandomEntityChoice : public EntityHandle {
 public:
  RandomEntityChoice(SkillContext* context, std::optional<int> id) : EntityHandle(context, id.value_or(0)), defined_(id.has_value()) {}
  bool defined() const { return defined_; }
  int id() const {
    if (!defined_) throw GameDataError("Cannot read id of an undefined random choice");
    return EntityHandle::id();
  }
 private:
  bool defined_;
};
// Overload the generated code's truthiness hook without changing generated card files.
namespace js {
inline bool truthy(const RandomEntityChoice& value) { return value.defined(); }
}
}  // namespace gitcg
