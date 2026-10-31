// An entity on the board or in a hand/pile (status, combat status, summon, support, equipment, event card),
// or an attachment on a card. Attachments are entities whose definition type is Attachment and that carry no
// attachments themselves (core/src/base/state.ts EntityState and AttachmentState).
#pragma once

#include <vector>

#include "core/state/ref_pointer.h"
#include "core/state/variables.h"

namespace gitcg {

struct EntityDefinition;

struct EntityState : RefCounted {
  int id = 0;
  const EntityDefinition* definition = nullptr;
  Variables variables;
  std::vector<RefPointer<EntityState>> attachments;
};

using EntityPointer = RefPointer<EntityState>;

}  // namespace gitcg
