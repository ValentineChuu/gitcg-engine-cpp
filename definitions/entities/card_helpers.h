// Shared target getters and play actions for card view-model attributes.
#pragma once
#include "definitions/skills/skill_model.h"
namespace gitcg {
ContextTargets characterTargets(std::function<bool(const CharacterHandle&, SkillContext&)> filter = {});
void setTalent(SkillModel& skill, EntityDefinition& card, const JsonValue& who, std::string requirement);
void setFood(SkillModel& skill, EntityDefinition& card, const Attribute& attribute);
void equipmentPlay(SkillModel& skill, const EntityModel& inner);
void supportPlay(SkillModel& skill, const EntityModel& inner);
}  // namespace gitcg
