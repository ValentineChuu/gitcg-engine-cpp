// Owns static definition objects and their skill records; pointers remain stable when the holder moves.
#pragma once

#include <memory>
#include "core/definitions/game_data.h"

namespace gitcg::testing {
struct LoadedDefinitions {
  GameData data;
  std::vector<std::unique_ptr<CharacterDefinition>> characters;
  std::vector<std::unique_ptr<EntityDefinition>> entities, attachments;
  std::vector<std::unique_ptr<ExtensionDefinition>> extensions;
  std::vector<std::unique_ptr<SkillDefinition>> skills;
  // Retains null/absent distinctions and description keys, whose executable values are not dumped.
  JsonValue source;
};
LoadedDefinitions loadDefinitions(const JsonValue& dump);
LoadedDefinitions loadDefinitions(const std::string& path);
}  // namespace gitcg::testing
