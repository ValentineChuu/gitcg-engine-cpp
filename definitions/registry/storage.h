// Stable ownership for registered versions and resolved characters; GameData borrows these objects.
#pragma once
#include <memory>
#include <unordered_map>
#include "definitions/attributes/attribute.h"
#include "definitions/attributes/lambda_slots.h"
#include "core/definitions/game_data.h"
#include "core/skills/skill_context.h"

namespace gitcg {
struct CharacterEntry {
  CharacterDefinition definition;
  std::vector<int> skillIds;
  int nightsoulId = 0;
};
struct PassiveEntry {
  int id = 0;
  VersionInfo version;
  std::vector<std::pair<Name, VariableConfig>> varConfigs;
  std::vector<const SkillDefinition*> skills;
};
struct InitiativeEntry {
  int id = 0;
  VersionInfo version;
  const SkillDefinition* skill = nullptr;
};
struct DefinitionStorage {
  std::shared_ptr<LambdaSlots> slots = std::make_shared<LambdaSlots>();
  std::unordered_map<std::string,std::unique_ptr<CustomEvent>> customEvents;
  std::vector<std::unique_ptr<SkillDefinition>> skills;
  std::vector<std::unique_ptr<EntityDefinition>> entities;
  std::vector<std::unique_ptr<EntityDefinition>> attachments;
  std::vector<std::unique_ptr<ExtensionDefinition>> extensions;
  std::vector<std::unique_ptr<CharacterEntry>> characters;
  std::vector<std::unique_ptr<InitiativeEntry>> initiatives;
  std::vector<std::unique_ptr<PassiveEntry>> passives;
  std::vector<std::unique_ptr<CharacterDefinition>> resolvedCharacters;
  double nextSubId(double mainId);
  const SkillDefinition* keepSkill(SkillDefinition skill);
  const CustomEvent* customEvent(const JsonValue& value);
};
}  // namespace gitcg
