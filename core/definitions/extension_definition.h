// A game-wide record kept by card data, e.g. which cards each player has played
// (core/src/base/extension.ts ExtensionDefinition).
#pragma once

#include <vector>

#include "core/definitions/skill_definition.h"
#include "core/definitions/version_info.h"
#include "core/json/json_value.h"

namespace gitcg {

struct ExtensionDefinition {
  int id = 0;
  VersionInfo version;
  JsonValue initialState;
  std::vector<const SkillDefinition*> skills;
};

}  // namespace gitcg
