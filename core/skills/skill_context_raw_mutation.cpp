#include "core/skills/skill_context.h"
#include "core/mutations/state_mutations.h"

namespace gitcg {

void SkillContext::mutateEntityVariable(int id, Name name, double value, int direction) {
  modifyEntityVariable(mutator_.mutableState(), id, name, value, direction);
}

}  // namespace gitcg
