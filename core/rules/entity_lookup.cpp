#include "core/rules/entity_lookup.h"

#include <stdexcept>
#include <string>

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"
#include "core/definitions/extension_definition.h"

namespace gitcg {

namespace {

struct ListArea {
  AreaType type;
  const std::vector<EntityPointer> PlayerState::*list;
};

constexpr ListArea LIST_AREAS[] = {
    {AreaType::CombatStatuses, &PlayerState::combatStatuses},
    {AreaType::Summons, &PlayerState::summons},
    {AreaType::Supports, &PlayerState::supports},
    {AreaType::Hands, &PlayerState::hands},
    {AreaType::Pile, &PlayerState::pile},
};

[[noreturn]] void throwNotFound(int id) {
  throw std::runtime_error("Cannot found entity " + std::to_string(id));
}

// Characters in response order: active first, then the rest in seat order after it.
std::vector<CharacterPointer> rotatedCharacters(const PlayerState& player) {
  size_t activeIndex = 0;
  for (size_t index = 0; index < player.characters.size(); ++index) {
    if (player.characters[index]->id == player.activeCharacterId) activeIndex = index;
  }
  std::vector<CharacterPointer> result;
  for (size_t offset = 0; offset < player.characters.size(); ++offset) {
    result.push_back(player.characters[(activeIndex + offset) % player.characters.size()]);
  }
  return result;
}

bool isAlive(const CharacterPointer& character) {
  return character->variables.valueOr(intern("alive"), 0) != 0;
}

}  // namespace

AnyState getEntityById(const GameState& state, int id) {
  for (const PlayerPointer& player : state.players) {
    for (const CharacterPointer& character : player->characters) {
      if (character->id == id) return character;
      for (const EntityPointer& entity : character->entities) {
        if (entity->id == id) return entity;
      }
    }
    for (const ListArea& area : LIST_AREAS) {
      for (const EntityPointer& entity : (*player).*(area.list)) {
        if (entity->id == id) return entity;
        for (const EntityPointer& attachment : entity->attachments) {
          if (attachment->id == id) return attachment;
        }
      }
    }
    for (const AnyState& removed : player->removedEntities) {
      if (removed.id() == id) return removed;
      if (removed.isEntity()) {
        for (const EntityPointer& attachment : removed.entity()->attachments) {
          if (attachment->id == id) return attachment;
        }
      }
    }
  }
  throwNotFound(id);
}

EntityArea getEntityArea(const GameState& state, int id) {
  for (int who = 0; who < 2; ++who) {
    const PlayerState& player = *state.players[who];
    for (const CharacterPointer& character : player.characters) {
      bool here = character->id == id;
      for (const EntityPointer& entity : character->entities) here = here || entity->id == id;
      if (here) return {AreaType::Characters, who, character->id, 0};
    }
    for (const ListArea& area : LIST_AREAS) {
      for (const EntityPointer& entity : player.*(area.list)) {
        bool here = entity->id == id;
        for (const EntityPointer& attachment : entity->attachments) here = here || attachment->id == id;
        if (here) return {area.type, who, 0, entity->id};
      }
    }
    for (const AnyState& removed : player.removedEntities) {
      bool here = removed.id() == id;
      if (removed.isEntity()) {
        for (const EntityPointer& attachment : removed.entity()->attachments) here = here || attachment->id == id;
      }
      if (here) return {AreaType::RemovedEntities, who, 0, removed.id()};
    }
  }
  throwNotFound(id);
}

int activeCharacterIndex(const PlayerState& player) {
  for (size_t index = 0; index < player.characters.size(); ++index) {
    if (player.characters[index]->id == player.activeCharacterId) return static_cast<int>(index);
  }
  throw std::runtime_error("Invalid active character id " + std::to_string(player.activeCharacterId));
}

const CharacterPointer& activeCharacter(const PlayerState& player) {
  return player.characters[activeCharacterIndex(player)];
}

std::vector<EntityWithArea> getAllEntities(const GameState& state, bool includeRemoved) {
  std::vector<EntityWithArea> result;
  const int order[2] = {state.currentTurn, 1 - state.currentTurn};
  std::vector<CharacterPointer> characters[2] = {rotatedCharacters(*state.players[0]), rotatedCharacters(*state.players[1])};
  auto appendCharacter = [&](const CharacterPointer& character, int who) {
    EntityArea area{AreaType::Characters, who, character->id, 0};
    result.push_back({character, area});
    for (const EntityPointer& entity : character->entities) result.push_back({entity, area});
  };
  for (int who : order) {
    for (const CharacterPointer& character : characters[who]) {
      if (!isAlive(character)) appendCharacter(character, who);
    }
  }
  for (int who : order) {
    const PlayerState& player = *state.players[who];
    const std::vector<CharacterPointer>& rotated = characters[who];
    if (!rotated.empty() && isAlive(rotated[0])) appendCharacter(rotated[0], who);
    for (const EntityPointer& status : player.combatStatuses) result.push_back({status, {AreaType::CombatStatuses, who, 0, 0}});
    for (size_t index = 1; index < rotated.size(); ++index) {
      if (isAlive(rotated[index])) appendCharacter(rotated[index], who);
    }
    for (const EntityPointer& summon : player.summons) result.push_back({summon, {AreaType::Summons, who, 0, 0}});
    for (const EntityPointer& support : player.supports) result.push_back({support, {AreaType::Supports, who, 0, 0}});
    for (const EntityPointer& card : player.hands) {
      EntityArea area{AreaType::Hands, who, 0, card->id};
      result.push_back({card, area});
      for (const EntityPointer& attachment : card->attachments) result.push_back({attachment, area});
    }
    for (auto card = player.pile.rbegin(); card != player.pile.rend(); ++card) {
      EntityArea area{AreaType::Pile, who, 0, (*card)->id};
      result.push_back({*card, area});
      for (const EntityPointer& attachment : (*card)->attachments) result.push_back({attachment, area});
    }
    if (includeRemoved) {
      for (const AnyState& removed : player.removedEntities) result.push_back({removed, {AreaType::RemovedEntities, who, 0, 0}});
    }
  }
  return result;
}

std::vector<EntityWithArea> getAllCharacters(const GameState& state) {
  std::vector<EntityWithArea> result;
  for (EntityWithArea& entry : getAllEntities(state)) {
    if (entry.state.isCharacter()) result.push_back(std::move(entry));
  }
  return result;
}

std::vector<AnyState> allEntitiesAtArea(const GameState& state, const EntityArea& area) {
  std::vector<AnyState> result;
  const PlayerState& player = *state.players[area.who];
  switch (area.type) {
    case AreaType::Characters:
      for (const CharacterPointer& character : player.characters) {
        if (character->id != area.characterId) continue;
        result.push_back(character);
        for (const EntityPointer& entity : character->entities) result.push_back(entity);
        return result;
      }
      throwNotFound(area.characterId);
    case AreaType::Hands:
    case AreaType::Pile:
      for (const EntityPointer& card : area.type == AreaType::Hands ? player.hands : player.pile) {
        if (card->id != area.cardId) continue;
        result.push_back(card);
        for (const EntityPointer& attachment : card->attachments) result.push_back(attachment);
        return result;
      }
      throwNotFound(area.cardId);
    case AreaType::CombatStatuses:
      for (const EntityPointer& entity : player.combatStatuses) result.push_back(entity);
      return result;
    case AreaType::Summons:
      for (const EntityPointer& entity : player.summons) result.push_back(entity);
      return result;
    case AreaType::Supports:
      for (const EntityPointer& entity : player.supports) result.push_back(entity);
      return result;
    case AreaType::RemovedEntities:
      return player.removedEntities;
  }
  return result;
}

const std::vector<const SkillDefinition*>& skillsOf(const AnyState& state) {
  return state.isCharacter() ? state.character()->definition->skills : state.entity()->definition->skills;
}

std::vector<CallerAndSkill> allSkills(const GameState& state, EventName triggerOn) {
  std::vector<CallerAndSkill> result;
  int activeId = state.players[state.currentTurn]->activeCharacterId;
  if (activeId != 0) {
    AnyState caller = getEntityById(state, activeId);
    for (const ExtensionState& extension : state.extensions) {
      for (const SkillDefinition* skill : extension.definition->skills) {
        if (skill->triggerOn == triggerOn) result.push_back({caller, skill});
      }
    }
  }
  std::vector<EntityWithArea> entities = triggerOn == EventName::OnBattleBegin ? getAllCharacters(state) : getAllEntities(state);
  for (const EntityWithArea& entry : entities) {
    for (const SkillDefinition* skill : skillsOf(entry.state)) {
      if (skill->triggerOn == triggerOn) result.push_back({entry.state, skill});
    }
  }
  return result;
}

}  // namespace gitcg
