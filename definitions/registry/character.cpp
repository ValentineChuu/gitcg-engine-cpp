// Character entries retain skill handles until version resolution assembles initiative and passive skills.
#include "definitions/skills/skill_model.h"
#include <stdexcept>

namespace gitcg {
void defineCharacter(DefinitionStorage& storage, const Attribute& root) {
  auto entry = std::make_unique<CharacterEntry>();
  auto& character = entry->definition;
  int health = 10, energy = 3;
  for (const auto& attribute : root.attributes) {
    const auto& name = attribute.name; const auto& value = attribute.argument(0);
    if (name == "id") character.id = value.asInt();
    else if (name == "since" || name == "until") character.version = {name == "since", value.asString()};
    else if (name == "tags") for (const auto& tag : attribute.positionals) character.tags.push_back(intern(tag.asString()));
    else if (name == "health") health = value.asInt();
    else if (name == "energy") energy = value.asInt();
    else if (name == "skills") for (const auto& id : attribute.positionals) entry->skillIds.push_back(id.asInt());
    else if (name == "associateNightsoul") entry->nightsoulId = value.asInt();
    else if (name == "enabledLunarReactions") for (const auto& reaction : attribute.positionals) character.enabledLunarReactions.push_back(reaction.asInt());
    else if (name == "specialEnergy") character.specialEnergy = SpecialEnergyConfig{intern(value.asString()), attribute.argument(1).asInt()};
    else throw std::runtime_error("Unknown character attribute " + name);
  }
  for (const auto& [name, value] : std::vector<std::pair<std::string, int>>{{"health", health}, {"energy", 0}, {"alive", 1}, {"aura", 0}, {"maxHealth", health}, {"maxEnergy", energy}})
    character.varConfigs.emplace_back(intern(name), createVariableConfig(value, JsonValue::object()));
  storage.characters.push_back(std::move(entry));
}
void defineCharacterSkill(DefinitionStorage& storage, const Attribute& root) {
  SkillModel skill(storage, OwnerType::Character);
  std::optional<VersionInfo> version;
  std::unique_ptr<PassiveEntry> passive;
  bool reserved = false;
  for (const auto& attribute : root.attributes) {
    const auto& name = attribute.name; const auto& value = attribute.argument(0);
    if (name == "reserved") reserved = true;
    else if (name == "since" || name == "until") version = VersionInfo{name == "since", value.asString()};
    else if (name == "skillType" && value.asString() == "passive") {
      EntityModel model(storage, OwnerType::Character, skill.id, skill.options);
      model.apply(attribute.attributes);
      if (model.reserved) { reserved = true; continue; }
      model.finishSkills();
      passive = std::make_unique<PassiveEntry>();
      passive->id = static_cast<int>(model.id); passive->version = model.definition.version;
      passive->skills = std::move(model.definition.skills); passive->varConfigs = std::move(model.definition.varConfigs);
    } else if (name == "skillType") {
      skill.type = skillType(value.asString());
      if (skill.type == SkillType::Burst) skill.config.gainEnergy = false;
    } else skill.apply({attribute});
  }
  if (reserved) return;
  if (passive) { if (version) passive->version = *version; storage.passives.push_back(std::move(passive)); }
  else {
    auto entry = std::make_unique<InitiativeEntry>();
    entry->id = static_cast<int>(skill.id); if (version) entry->version = *version;
    entry->skill = storage.keepSkill(skill.buildInitiative()); storage.initiatives.push_back(std::move(entry));
  }
}
}  // namespace gitcg
