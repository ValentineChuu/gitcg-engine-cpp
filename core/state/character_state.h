// A character and the statuses and equipment on it (core/src/base/state.ts CharacterState).
#pragma once

#include <vector>

#include "core/state/entity_state.h"
#include "core/state/ref_pointer.h"
#include "core/state/variables.h"

namespace gitcg {

struct CharacterDefinition;

struct CharacterState : RefCounted {
  int id = 0;
  const CharacterDefinition* definition = nullptr;
  std::vector<EntityPointer> entities;
  Variables variables;
};

using CharacterPointer = RefPointer<CharacterState>;

}  // namespace gitcg
