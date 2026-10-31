// Finding entities in a game state (core/src/utils.ts): by id, their area, and everything on the board in the
// order skills respond to events.
#pragma once

#include <vector>

#include "core/definitions/event_name.h"
#include "core/state/any_state.h"
#include "core/state/entity_area.h"
#include "core/state/game_state.h"

namespace gitcg {

struct SkillDefinition;

// Throws when the id is nowhere (characters, their entities, combat statuses, summons, supports, hands, pile,
// removed entities, and attachments of all of these).
AnyState getEntityById(const GameState& state, int id);
EntityArea getEntityArea(const GameState& state, int id);

int activeCharacterIndex(const PlayerState& player);  // throws when the active id isn't a character
const CharacterPointer& activeCharacter(const PlayerState& player);

struct EntityWithArea {
  AnyState state;
  EntityArea area;
};

// Every entity in response order: defeated characters first (current turn's player first), then per player
// (current turn first): active character and its entities, combat statuses, standby characters and theirs,
// summons, supports, hands (with attachments), pile from the bottom (with attachments), optionally removed ones.
std::vector<EntityWithArea> getAllEntities(const GameState& state, bool includeRemoved = false);

// All characters, for onBattleBegin (hostRelatedExecution is never on).
std::vector<EntityWithArea> getAllCharacters(const GameState& state);

// Entities in one area: a character and its entities, a card and its attachments, or a whole list.
std::vector<AnyState> allEntitiesAtArea(const GameState& state, const EntityArea& area);

struct CallerAndSkill {
  AnyState caller;
  const SkillDefinition* skill;
};

// Every skill triggered by `triggerOn`: extensions first (called from the current player's active character, only
// when one is set), then entities in response order. Removed entities are left out.
std::vector<CallerAndSkill> allSkills(const GameState& state, EventName triggerOn);

// Character and entity definitions both carry skills.
const std::vector<const SkillDefinition*>& skillsOf(const AnyState& state);

}  // namespace gitcg
