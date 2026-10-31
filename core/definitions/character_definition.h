// What a character is (core/src/base/character.ts CharacterDefinition).
#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/definitions/skill_definition.h"
#include "core/definitions/variable_config.h"
#include "core/definitions/version_info.h"
#include "core/values/game_enums.h"
#include "core/values/names.h"

namespace gitcg {

struct EntityDefinition;

struct SpecialEnergyConfig {
  Name variableName;
  int slotSize = 0;
};

struct CharacterDefinition {
  int id = 0;
  VersionInfo version;
  std::vector<Name> tags;
  std::vector<std::pair<Name, VariableConfig>> varConfigs;
  std::vector<const SkillDefinition*> skills;
  const EntityDefinition* associatedNightsoulsBlessing = nullptr;
  std::vector<Reaction> enabledLunarReactions;
  std::optional<SpecialEnergyConfig> specialEnergy;

  bool hasTag(Name tag) const {
    for (Name own : tags) {
      if (own == tag) return true;
    }
    return false;
  }
  const VariableConfig* findVarConfig(Name name) const {
    for (const auto& [key, config] : varConfigs) {
      if (key == name) return &config;
    }
    return nullptr;
  }
};

}  // namespace gitcg
