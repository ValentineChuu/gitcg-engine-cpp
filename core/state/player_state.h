// One player's side of the game (core/src/base/state.ts PlayerState).
#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "core/state/any_state.h"
#include "core/state/character_state.h"
#include "core/state/entity_state.h"
#include "core/state/ref_pointer.h"
#include "core/values/game_enums.h"

namespace gitcg {

struct EntityDefinition;
class DamageOrHealEventArg;
class ReactionEventArg;

enum class PlayerFlag { DeclaredEnd, HasDefeated, CanCharged, CanPlunging, LegendUsed, SkipNextTurn, DefeatedSwitching };

struct PlayerState : RefCounted {
  int who = 0;
  std::vector<const EntityDefinition*> initialPile;
  std::vector<EntityPointer> pile;
  int activeCharacterId = 0;
  std::vector<EntityPointer> hands;
  std::vector<CharacterPointer> characters;
  std::vector<EntityPointer> combatStatuses;
  std::vector<EntityPointer> supports;
  std::vector<EntityPointer> summons;
  std::vector<DiceType> dice;
  bool declaredEnd = false;
  bool hasDefeated = false;
  bool canCharged = false;
  bool canPlunging = false;
  bool legendUsed = false;
  bool skipNextTurn = false;
  bool defeatedSwitching = false;
  // Per round: character definition id -> skill ids used (a JS Map, so insertion ordered).
  std::vector<std::pair<int, std::vector<double>>> roundSkillLog;
  // This phase's damage and reaction events caused by this player (event args, shared with the event lists).
  std::vector<std::shared_ptr<DamageOrHealEventArg>> phaseDamageLog;
  std::vector<std::shared_ptr<ReactionEventArg>> phaseReactionLog;
  std::vector<AnyState> removedEntities;

  bool flag(PlayerFlag name) const;
  void setFlag(PlayerFlag name, bool value);
};

using PlayerPointer = RefPointer<PlayerState>;

}  // namespace gitcg
