// Every definition of the chosen game version, by id (core/src/data/registry.ts GameData). The lists keep the
// JS Map order (first registration of each id), which random card choices over all definitions depend on.
#pragma once

#include <unordered_map>
#include <vector>

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"
#include "core/definitions/extension_definition.h"

namespace gitcg {

template <typename Definition>
class DefinitionMap {
 public:
  void add(const Definition* definition) {
    if (byId_.emplace(definition->id, definition).second) ordered_.push_back(definition);
  }
  const Definition* find(int id) const {
    auto found = byId_.find(id);
    return found == byId_.end() ? nullptr : found->second;
  }
  const std::vector<const Definition*>& values() const { return ordered_; }

 private:
  std::unordered_map<int, const Definition*> byId_;
  std::vector<const Definition*> ordered_;
};

struct GameData {
  DefinitionMap<ExtensionDefinition> extensions;
  DefinitionMap<EntityDefinition> entities;
  DefinitionMap<CharacterDefinition> characters;
  DefinitionMap<EntityDefinition> attachments;
};

}  // namespace gitcg
