#include "core/events/event_arg.h"

#include <algorithm>
#include <stdexcept>

namespace gitcg {

const AnyState& EventArg::caller() const {
  if (!currentSkillInfo) throw std::logic_error("EventArg caller not set");
  return currentSkillInfo->caller;
}

void ModifyRollEventArg::fixDice(DiceType type, int count) {
  int room = std::max(0, onTimeState->config.initialDiceCount - static_cast<int>(fixedDice.size()));
  fixedDice.insert(fixedDice.end(), std::min(count, room), type);
}

bool isCharacterInitiativeSkill(const SkillInfo& skill, bool allowTechnique) {
  if (skill.definition->triggerOn != EventName::Initiative) return false;
  switch (skill.definition->skillType) {
    case SkillType::Normal:
    case SkillType::Elemental:
    case SkillType::Burst: return true;
    case SkillType::Technique: return allowTechnique;
    default: return false;
  }
}

}  // namespace gitcg
