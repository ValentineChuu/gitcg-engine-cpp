#include "core/rules/initiative_skills.h"

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"

namespace gitcg {

std::vector<CallerAndSkill> initiativeSkillsOfPlayer(const PlayerState& player, bool includeHidden) {
  std::vector<CallerAndSkill> result;
  const CharacterPointer* active = nullptr;
  for (const CharacterPointer& character : player.characters) {
    if (character->id == player.activeCharacterId) active = &character;
  }
  if (!active) return result;
  for (const EntityPointer& entity : (*active)->entities) {
    for (const SkillDefinition* skill : entity->definition->skills) {
      if (skill->triggerOn == EventName::Initiative && skill->skillType != SkillType::PlayCard &&
          (includeHidden || !skill->initiativeSkillConfig->hidden)) {
        result.push_back({entity, skill});
      }
    }
  }
  for (const SkillDefinition* skill : (*active)->definition->skills) {
    if (skill->triggerOn == EventName::Initiative && (includeHidden || !skill->initiativeSkillConfig->hidden)) {
      result.push_back({*active, skill});
    }
  }
  return result;
}

bool isSkillDisabled(const CharacterState& character) {
  static const Name DISABLE_SKILL = intern("disableSkill");
  for (const EntityPointer& entity : character.entities) {
    if (entity->definition->hasTag(DISABLE_SKILL)) return true;
  }
  return false;
}

ChargedPlunging chargedPlunging(const SkillDefinition& skill, const PlayerState& player) {
  static const Name NORMAL_AS_PLUNGING = intern("normalAsPlunging");
  if (!skill.initiativeSkillConfig) return {};
  ChargedPlunging result{skill.initiativeSkillConfig->alwaysCharged, skill.initiativeSkillConfig->alwaysPlunging};
  if (skill.skillType != SkillType::Normal) return result;
  const CharacterPointer& active = activeCharacter(player);
  bool asPlunging = false;
  for (const EntityPointer& entity : active->entities) asPlunging = asPlunging || entity->definition->hasTag(NORMAL_AS_PLUNGING);
  result.charged = result.charged || player.canCharged;
  result.plunging = result.plunging || player.canPlunging || asPlunging;
  return result;
}

std::vector<InitiativeSkillArgument> playCardTargetCandidates(const GameStatePointer& state, int who, const EntityPointer& card,
                                                              const SkillInfo& skill) {
  const PlayerState& player = *state->players[who];
  if (card->definition->type == EntityType::Support && static_cast<int>(player.supports.size()) == state->config.maxSupportsCount) {
    std::vector<InitiativeSkillArgument> result;
    for (const EntityPointer& support : player.supports) result.emplace_back(std::vector<AnyState>{support});
    return result;
  }
  return skill.definition->initiativeSkillConfig->getTarget(state, skill);
}

}  // namespace gitcg
