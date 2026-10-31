#include "core/mutations/state_mutations.h"
#include "core/mutations/mutation_observer.h"
#include "core/mutations/notifications.h"
#include "core/expose/expose_state.h"

#include <stdexcept>
#include <string>
#include <utility>

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"
#include "core/definitions/extension_definition.h"
#include "core/values/usage_per_round_names.h"
#include "core/rules/dice_order.h"
#include "core/rules/entity_lookup.h"
#include "core/rules/random.h"

namespace gitcg {

namespace {

using EntityList = std::vector<EntityPointer>;

// Lists searched (in this order) by removeEntity and by id lookups.
EntityList* listOf(PlayerState& player, AreaType type) {
  switch (type) {
    case AreaType::CombatStatuses: return &player.combatStatuses;
    case AreaType::Summons: return &player.summons;
    case AreaType::Supports: return &player.supports;
    case AreaType::Hands: return &player.hands;
    case AreaType::Pile: return &player.pile;
    default: return nullptr;
  }
}

constexpr AreaType LIST_TYPES[] = {AreaType::CombatStatuses, AreaType::Summons, AreaType::Supports, AreaType::Hands, AreaType::Pile};

[[noreturn]] void throwNotFound(int id) {
  throw std::runtime_error("Cannot found entity " + std::to_string(id));
}

PlayerState& editPlayer(GameStatePointer& state, int who) {
  return editable(editable(state).players[who]);
}

// The editable node with this id (copying its path), searched like getEntityById. Exactly one of the two results
// is set.
struct EditedNode {
  CharacterState* character = nullptr;
  EntityState* entity = nullptr;
  Variables& variables() { return character ? character->variables : entity->variables; }
};

EditedNode editNode(GameStatePointer& state, int id) {
  // Find first (without copying), then copy only the path to it.
  EntityArea area = getEntityArea(*state, id);
  PlayerState& player = editPlayer(state, area.who);
  if (area.type == AreaType::Characters) {
    for (CharacterPointer& characterPointer : player.characters) {
      if (characterPointer->id != area.characterId) continue;
      CharacterState& character = editable(characterPointer);
      if (character.id == id) return {&character, nullptr};
      for (EntityPointer& entity : character.entities) {
        if (entity->id == id) return {nullptr, &editable(entity)};
      }
    }
  } else if (area.type == AreaType::RemovedEntities) {
    for (AnyState& removed : player.removedEntities) {
      if (removed.id() != area.cardId) continue;
      if (removed.isCharacter()) {
        CharacterPointer copy = removed.character();
        CharacterState& character = editable(copy);
        removed = AnyState(copy);
        return {&character, nullptr};
      }
      EntityPointer copy = removed.entity();
      EntityState& entity = editable(copy);
      removed = AnyState(copy);
      if (entity.id == id) return {nullptr, &entity};
      for (EntityPointer& attachment : entity.attachments) {
        if (attachment->id == id) return {nullptr, &editable(attachment)};
      }
    }
  } else {
    for (EntityPointer& entityPointer : *listOf(player, area.type)) {
      if (entityPointer->id != area.cardId) continue;
      EntityState& entity = editable(entityPointer);
      if (entity.id == id) return {nullptr, &entity};
      for (EntityPointer& attachment : entity.attachments) {
        if (attachment->id == id) return {nullptr, &editable(attachment)};
      }
    }
  }
  throwNotFound(id);
}

// Takes an entity or attachment out of the board, hands or pile (not removed entities), like utils.removeEntity.
EntityPointer takeOut(GameStatePointer& state, int id) {
  for (int who = 0; who < 2; ++who) {
    const PlayerState& reading = *state->players[who];
    for (size_t characterIndex = 0; characterIndex < reading.characters.size(); ++characterIndex) {
      const auto& entities = reading.characters[characterIndex]->entities;
      for (size_t index = 0; index < entities.size(); ++index) {
        if (entities[index]->id != id) continue;
        CharacterState& character = editable(editPlayer(state, who).characters[characterIndex]);
        EntityPointer removed = character.entities[index];
        character.entities.erase(character.entities.begin() + index);
        return removed;
      }
    }
    for (AreaType type : LIST_TYPES) {
      const EntityList& list = *listOf(const_cast<PlayerState&>(reading), type);
      for (size_t index = 0; index < list.size(); ++index) {
        if (list[index]->id == id) {
          EntityList& editableList = *listOf(editPlayer(state, who), type);
          EntityPointer removed = editableList[index];
          editableList.erase(editableList.begin() + index);
          return removed;
        }
        if (type != AreaType::Hands && type != AreaType::Pile) continue;
        const auto& attachments = list[index]->attachments;
        for (size_t attachmentIndex = 0; attachmentIndex < attachments.size(); ++attachmentIndex) {
          if (attachments[attachmentIndex]->id != id) continue;
          EntityState& card = editable((*listOf(editPlayer(state, who), type))[index]);
          EntityPointer removed = card.attachments[attachmentIndex];
          card.attachments.erase(card.attachments.begin() + attachmentIndex);
          return removed;
        }
      }
    }
  }
  throwNotFound(id);
}

void insertAt(EntityList& list, EntityPointer entity, std::optional<int> targetIndex) {
  if (!targetIndex || *targetIndex < 0 || *targetIndex > static_cast<int>(list.size())) {
    list.push_back(std::move(entity));
  } else {
    list.insert(list.begin() + *targetIndex, std::move(entity));
  }
}

EntityList& entitiesAt(GameStatePointer& state, const EntityArea& area) {
  PlayerState& player = editPlayer(state, area.who);
  if (area.type == AreaType::Characters) {
    for (CharacterPointer& character : player.characters) {
      if (character->id == area.characterId) return editable(character).entities;
    }
    throw std::runtime_error("Character " + std::to_string(area.characterId) + " not found");
  }
  return *listOf(player, area.type);
}

// A mutation the person's board isn't shown, still counted (a notification goes out when any mutation is pending).
void recordHidden() {
  if (notificationSink) notificationSink->stateMutation(std::nullopt);
}
void record(const char* kind, JsonValue value) {
  notificationSink->stateMutation(exposedMutation(kind, std::move(value)));
}
JsonValue emptyValue() { return JsonValue::object(); }

}  // namespace

int64_t stepRandom(GameStatePointer& state) {
  recordHidden();
  GameState& game = editable(state);
  game.randomIterator = nextRandom(game.randomIterator);
  return game.randomIterator;
}

int stepId(GameStatePointer& state) {
  recordHidden();
  GameState& game = editable(state);
  return game.idIterator--;
}

bool changePhase(GameStatePointer& state, Phase newPhase) {
  GameState& game = editable(state);
  game.previousPhase = game.phase;
  bool changed = game.phase != newPhase;
  game.phase = newPhase;
  if (notificationSink) {
    JsonValue value = JsonValue::object();
    value["hasChange"] = changed;
    value["newPhase"] = static_cast<int>(newPhase);
    record("changePhase", std::move(value));
  }
  return changed;
}

void stepRound(GameStatePointer& state) {
  ++editable(state).roundNumber;
  if (notificationSink) record("stepRound", emptyValue());
}

void switchTurn(GameStatePointer& state) {
  GameState& game = editable(state);
  game.currentTurn = 1 - game.currentTurn;
  if (notificationSink) record("switchTurn", emptyValue());
}

void setWinner(GameStatePointer& state, int winner) {
  editable(state).winner = winner;
  if (notificationSink) {
    JsonValue value = JsonValue::object();
    value["winner"] = winner;
    record("setWinner", std::move(value));
  }
}

void setActiveCharacter(GameStatePointer& state, int who, int characterId) {
  recordHidden();  // the "switchActive" mutation; Game notifies the switch itself
  editPlayer(state, who).activeCharacterId = characterId;
}

void swapCharacterPosition(GameStatePointer& state, int who, int firstId, int secondId) {
  PlayerState& player = editPlayer(state, who);
  auto first = std::find_if(player.characters.begin(), player.characters.end(), [&](const CharacterPointer& c) { return c->id == firstId; });
  auto second = std::find_if(player.characters.begin(), player.characters.end(), [&](const CharacterPointer& c) { return c->id == secondId; });
  if (first == player.characters.end() || second == player.characters.end()) {
    throw std::logic_error("Character not found in player " + std::to_string(who));
  }
  if (notificationSink) {
    JsonValue value = JsonValue::object();
    value["who"] = who;
    value["character0Id"] = (*first)->id;
    value["character0DefinitionId"] = (*first)->definition->id;
    value["character1Id"] = (*second)->id;
    value["character1DefinitionId"] = (*second)->definition->id;
    record("swapCharacterPosition", std::move(value));
  }
  std::swap(*first, *second);
}

int createEntity(GameStatePointer& state, const EntityArea& area, EntityState value, std::optional<int> targetIndex) {
  if (value.id == 0) value.id = editable(state).idIterator--;
  int id = value.id;
  EntityList& list = entitiesAt(state, area);
  if (area.type == AreaType::Characters) {
    list.push_back(makeRef<EntityState>(std::move(value)));
  } else {
    insertAt(list, makeRef<EntityState>(std::move(value)), targetIndex);
  }
  if (mutationObserver) {
    const EntityState& entity = *getEntityById(*state, id).entity();
    JsonValue value = JsonValue::object();
    value["who"] = area.who;
    value["where"] = exposedArea(area.type);
    value["entity"] = exposeEntityForMutation(entity, area.who != 0 && area.type == AreaType::Hands);
    if (area.type == AreaType::Characters) value["masterCharacterId"] = area.characterId;
    observeMutation("createEntity", std::move(value));
  }
  recordEntityCreated(state, area, id);
  return id;
}

void recordEntityCreated(const GameStatePointer& state, const EntityArea& area, int id) {
  if (!notificationSink) return;
  const EntityState& entity = *getEntityById(*state, id).entity();
  JsonValue value = JsonValue::object();
  value["who"] = area.who;
  value["where"] = exposedArea(area.type);
  value["entity"] = exposeEntityForMutation(entity, area.who != notificationSink->seat() && area.type == AreaType::Hands);
  if (area.type == AreaType::Characters) value["masterCharacterId"] = area.characterId;
  record("createEntity", std::move(value));
}

int createAttachment(GameStatePointer& state, int cardId, EntityState value) {
  if (value.id == 0) value.id = editable(state).idIterator--;
  int id = value.id;
  EditedNode card = editNode(state, cardId);
  card.entity->attachments.push_back(makeRef<EntityState>(std::move(value)));
  if (notificationSink) {
    EntityArea area = getEntityArea(*state, cardId);
    JsonValue exposed = JsonValue::object();
    exposed["who"] = area.who;
    exposed["where"] = exposedArea(area.type);
    exposed["attachment"] = exposeAttachmentForMutation(*card.entity->attachments.back());
    exposed["masterCardId"] = cardId;
    record("createAttachment", std::move(exposed));
  }
  return id;
}

void moveEntity(GameStatePointer& state, int id, const EntityArea& area, std::optional<int> targetIndex, MoveReason reason,
                const EntityState* passedValue) {
  EntityArea from;
  if (mutationObserver || notificationSink) from = getEntityArea(*state, id);
  EntityPointer latest = takeOut(state, id);
  insertAt(entitiesAt(state, area), latest, targetIndex);
  if (mutationObserver) {
    JsonValue value = JsonValue::object();
    value["fromWho"] = from.who;
    value["fromWhere"] = exposedArea(from.type);
    value["toWho"] = area.who;
    value["toWhere"] = exposedArea(area.type);
    if (area.type == AreaType::Characters) value["toMasterCharacterId"] = area.characterId;
    if (targetIndex) value["targetIndex"] = *targetIndex;
    value["entity"] = exposeEntityForMutation(passedValue ? *passedValue : *latest,
                                              from.who != 0 && (area.type == AreaType::Hands || area.type == AreaType::Pile));
    value["reason"] = exposedMoveReason(reason);
    observeMutation("moveEntity", std::move(value));
  }
  if (notificationSink) {
    JsonValue value = JsonValue::object();
    value["fromWho"] = from.who;
    value["fromWhere"] = exposedArea(from.type);
    value["toWho"] = area.who;
    value["toWhere"] = exposedArea(area.type);
    if (area.type == AreaType::Characters) value["toMasterCharacterId"] = area.characterId;
    if (targetIndex) value["targetIndex"] = *targetIndex;
    bool hidden = from.who != notificationSink->seat() && (area.type == AreaType::Hands || area.type == AreaType::Pile);
    value["entity"] = exposeEntityForMutation(passedValue ? *passedValue : *latest, hidden);
    value["reason"] = exposedMoveReason(reason);
    record("moveEntity", std::move(value));
  }
}

void removeEntity(GameStatePointer& state, int id, int fromWho, RemoveReason reason) {
  EntityArea from;
  if (mutationObserver || notificationSink) from = getEntityArea(*state, id);
  EntityPointer removed = takeOut(state, id);
  editPlayer(state, fromWho).removedEntities.push_back(AnyState(removed));
  if (mutationObserver && removed->definition->type != EntityType::Attachment) {
    JsonValue value = JsonValue::object();
    value["who"] = from.who;
    value["where"] = exposedArea(from.type);
    bool hidden = from.who != 0 && (from.type == AreaType::Hands || from.type == AreaType::Pile) &&
                  (reason == RemoveReason::Overflow || reason == RemoveReason::ElementalTuning);
    value["entity"] = exposeEntityForMutation(*removed, hidden);
    value["reason"] = exposedRemoveReason(reason);
    observeMutation("removeEntity", std::move(value));
  }
  if (notificationSink) {
    if (removed->definition->type == EntityType::Attachment) {
      recordHidden();
    } else {
      JsonValue value = JsonValue::object();
      value["who"] = from.who;
      value["where"] = exposedArea(from.type);
      bool hidden = from.who != notificationSink->seat() && (from.type == AreaType::Hands || from.type == AreaType::Pile) &&
                    (reason == RemoveReason::Overflow || reason == RemoveReason::ElementalTuning);
      value["entity"] = exposeEntityForMutation(*removed, hidden);
      value["reason"] = exposedRemoveReason(reason);
      record("removeEntity", std::move(value));
    }
  }
}

double modifyEntityVariable(GameStatePointer& state, int id, Name name, double value, int direction) {
  if (value > DEFAULT_VARIABLE_UPPER_BOUND || value < DEFAULT_VARIABLE_LOWER_BOUND) return 0;
  AnyState oldState = getEntityById(*state, id);
  const auto old = oldState.variables().find(name);
  double oldValue = old.value_or(0);
  if (!old || *old != value) editNode(state, id).variables().set(name, value);
  if (mutationObserver) {
    AnyState latest = getEntityById(*state, id);
    bool visible = false;
    std::string variableName = nameText(name);
    if (latest.isCharacter()) {
      visible = variableName == "health" || variableName == "energy" || variableName == "aura" || variableName == "alive";
      const auto& special = latest.character()->definition->specialEnergy;
      if (special && special->variableName == name) { visible = true; variableName = "energy"; }
    } else {
      visible = latest.entity()->definition->visibleVarName == name;
    }
    if (visible) {
      JsonValue mutation = JsonValue::object();
      mutation["entityId"] = id;
      mutation["entityDefinitionId"] = latest.isCharacter() ? latest.character()->definition->id : latest.entity()->definition->id;
      mutation["variableName"] = variableName;
      mutation["variableValue"] = value;
      mutation["oldValue"] = oldValue;
      mutation["direction"] = direction;
      // Private metadata used only by the preview collector to preserve the grouping of the JS maps.
      mutation["_group"] = latest.isCharacter() ? variableName : "visible";
      observeMutation("modifyEntityVar", std::move(mutation));
    }
  }
  if (notificationSink) {
    JsonValue mutation = JsonValue::object();
    mutation["entityId"] = id;
    mutation["entityDefinitionId"] = oldState.isCharacter() ? oldState.character()->definition->id : oldState.entity()->definition->id;
    mutation["variableName"] = nameText(name);
    mutation["variableValue"] = value;
    mutation["oldValue"] = oldValue;
    mutation["direction"] = direction;
    record("modifyEntityVar", std::move(mutation));
  }
  return oldValue;
}

void transformCharacterDefinition(GameStatePointer& state, int id, const CharacterDefinition* definition) {
  int who = getEntityArea(*state, id).who;
  EditedNode node = editNode(state, id);
  int oldDefinitionId = node.character->definition->id;
  node.character->definition = definition;
  if (notificationSink) {
    JsonValue value = JsonValue::object();
    value["entityId"] = id;
    value["newEntityDefinitionId"] = definition->id;
    record("transformDefinition", std::move(value));
  }
  // The round's skill log moves to the new definition id.
  auto& log = editPlayer(state, who).roundSkillLog;
  std::vector<double> used;
  for (auto entry = log.begin(); entry != log.end(); ++entry) {
    if (entry->first == oldDefinitionId) used = entry->second;
  }
  bool replaced = false;
  for (auto& [definitionId, skills] : log) {
    if (definitionId == definition->id) {
      skills = used;
      replaced = true;
    }
  }
  if (!replaced) log.emplace_back(definition->id, used);
  std::erase_if(log, [&](const auto& entry) { return entry.first == oldDefinitionId; });
}

void transformEntityDefinition(GameStatePointer& state, int id, const EntityDefinition* definition) {
  editNode(state, id).entity->definition = definition;
  if (notificationSink) {
    JsonValue value = JsonValue::object();
    value["entityId"] = id;
    value["newEntityDefinitionId"] = definition->id;
    record("transformDefinition", std::move(value));
  }
}

void resetVariables(GameStatePointer& state, int id, bool usagePerRoundOnly) {
  EditedNode node = editNode(state, id);
  if (notificationSink) {
    if (usagePerRoundOnly) {
      JsonValue value = JsonValue::object();
      value["entityId"] = id;
      value["entityDefinitionId"] = node.character ? node.character->definition->id : node.entity->definition->id;
      record("resetVariables", std::move(value));
    } else {
      recordHidden();
    }
  }
  const auto& configs = node.character ? node.character->definition->varConfigs : node.entity->definition->varConfigs;
  for (const auto& [name, config] : configs) {
    if (usagePerRoundOnly && !isUsagePerRoundName(name)) continue;
    node.variables().set(name, config.initialValue);
  }
}

void resetDice(GameStatePointer& state, int who, const std::vector<DiceType>& dice, DiceReason reason,
               std::optional<DiceType> conversionTargetHint) {
  std::vector<DiceType> sorted = sortDice(*state->players[who], dice);
  if (notificationSink) {
    JsonValue values = JsonValue::array();
    // As passed: the core records the caller's value (paying a cost leaves the remaining dice unsorted).
    for (DiceType die : dice) values.push(who == notificationSink->seat() ? static_cast<int>(die) : 0);
    JsonValue value = JsonValue::object();
    value["who"] = who;
    value["dice"] = std::move(values);
    value["reason"] = static_cast<int>(reason);
    if (conversionTargetHint) value["conversionTargetHint"] = static_cast<int>(*conversionTargetHint);
    record("resetDice", std::move(value));
  }
  editPlayer(state, who).dice = std::move(sorted);
}

void setPlayerFlag(GameStatePointer& state, int who, PlayerFlag flag, bool value) {
  if (notificationSink) {
    // Recorded before the no-change check: the core mutates (and notifies) even when the flag keeps its value.
    if (flag == PlayerFlag::DeclaredEnd || flag == PlayerFlag::LegendUsed) {
      JsonValue exposed = JsonValue::object();
      exposed["who"] = who;
      exposed["flagName"] = flag == PlayerFlag::DeclaredEnd ? 1 : 2;
      exposed["flagValue"] = value;
      record("setPlayerFlag", std::move(exposed));
    } else {
      recordHidden();
    }
  }
  if (state->players[who]->flag(flag) == value) return;
  editPlayer(state, who).setFlag(flag, value);
}

namespace {
thread_local bool extensionContainerReplaced = false;
}  // namespace

void noteExtensionContainerReplaced() { extensionContainerReplaced = true; }
bool takeExtensionContainerReplaced() { return std::exchange(extensionContainerReplaced, false); }

void mutateExtensionState(GameStatePointer& state, int extensionId, JsonValue newState, bool replacedContainer) {
  recordHidden();
  for (const ExtensionState& extension : state->extensions) {
    if (extension.definition->id == extensionId && !replacedContainer && extension.state == newState) return;
  }
  for (ExtensionState& extension : editable(state).extensions) {
    if (extension.definition->id == extensionId) {
      extension.state = std::move(newState);
      return;
    }
  }
  throw std::logic_error("Extension " + std::to_string(extensionId) + " not found in state");
}

void pushRoundSkillLog(GameStatePointer& state, int callerDefinitionId, int callerId, double skillId) {
  recordHidden();
  int who = getEntityArea(*state, callerId).who;
  auto& log = editPlayer(state, who).roundSkillLog;
  for (auto& [definitionId, skills] : log) {
    if (definitionId == callerDefinitionId) {
      skills.push_back(skillId);
      return;
    }
  }
  log.emplace_back(callerDefinitionId, std::vector<double>{skillId});
}

void removeRoundSkillLog(GameStatePointer& state, int callerDefinitionId, int callerId) {
  recordHidden();
  int who = getEntityArea(*state, callerId).who;
  std::erase_if(editPlayer(state, who).roundSkillLog, [&](const auto& entry) { return entry.first == callerDefinitionId; });
}

void clearRoundLogs(GameStatePointer& state) {
  recordHidden();
  editPlayer(state, 0).roundSkillLog.clear();
  editPlayer(state, 1).roundSkillLog.clear();
}

void clearRemovedEntities(GameStatePointer& state) {
  recordHidden();
  editPlayer(state, 0).removedEntities.clear();
  editPlayer(state, 1).removedEntities.clear();
}

void pushPhaseDamageLog(GameStatePointer& state, int sourceWho, std::shared_ptr<DamageOrHealEventArg> damageEvent) {
  recordHidden();
  editPlayer(state, sourceWho).phaseDamageLog.push_back(std::move(damageEvent));
}

void pushPhaseReactionLog(GameStatePointer& state, int viaWho, std::shared_ptr<ReactionEventArg> reactionEvent) {
  recordHidden();
  editPlayer(state, viaWho).phaseReactionLog.push_back(std::move(reactionEvent));
}

void clearPhaseLogs(GameStatePointer& state) {
  recordHidden();
  for (int who = 0; who < 2; ++who) {
    PlayerState& player = editPlayer(state, who);
    player.phaseDamageLog.clear();
    player.phaseReactionLog.clear();
  }
}

}  // namespace gitcg
