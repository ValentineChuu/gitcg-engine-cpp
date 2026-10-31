// Where an entity is (core/src/base/entity.ts EntityArea).
#pragma once

namespace gitcg {

enum class AreaType { Characters, CombatStatuses, Summons, Supports, Hands, Pile, RemovedEntities };

struct EntityArea {
  AreaType type = AreaType::Characters;
  int who = 0;
  int characterId = 0;  // Characters
  int cardId = 0;       // Hands, Pile (0 when unspecified); getEntityArea also fills it for the other lists

  bool operator==(const EntityArea& other) const = default;
};

}  // namespace gitcg
