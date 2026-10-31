#include "core/expose/expose_state.h"

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"
#include "core/rules/costs.h"
#include "core/rules/initiative_skills.h"
#include "core/values/usage_per_round_names.h"

namespace gitcg {

namespace {

constexpr int CHARACTER_TAG_SHIELD = 1 << 0;
constexpr int CHARACTER_TAG_BARRIER = 1 << 1;
constexpr int CHARACTER_TAG_DISABLE_SKILL = 1 << 2;
constexpr int CHARACTER_TAG_NIGHTSOULS_BLESSING = 1 << 3;
constexpr int CHARACTER_TAG_BOND_OF_LIFE = 1 << 4;
constexpr int CARD_TAG_ABYSS = 1 << 0;
constexpr int CARD_TAG_CONDUCTIVE = 1 << 1;

int exposedEntityType(EntityType type) {
  switch (type) {
    case EntityType::EventCard: return 1;
    case EntityType::Status: return 2;
    case EntityType::CombatStatus: return 3;
    case EntityType::Summon: return 4;
    case EntityType::Support: return 5;
    case EntityType::Equipment: return 6;
    case EntityType::Attachment: return 0;
  }
  return 0;
}

JsonValue exposeDiceRequirement(const DiceRequirement& requirement) {
  JsonValue result = JsonValue::array();
  for (const auto& [type, count] : requirement.entries()) {
    JsonValue entry = JsonValue::object();
    entry["type"] = type;
    entry["count"] = count;
    result.push(std::move(entry));
  }
  return result;
}

JsonValue descriptionsOf(const EntityDefinition& definition, const GameStatePointer& state, int id) {
  JsonValue dictionary = JsonValue::object();
  for (const auto& [key, entry] : definition.descriptionDictionary) dictionary[key] = entry(state, id);
  return dictionary;
}

JsonValue exposeAttachment(const GameStatePointer& state, const EntityState& attachment) {
  JsonValue result = JsonValue::object();
  result["id"] = attachment.id;
  result["definitionId"] = attachment.definition->id;
  result["descriptionDictionary"] = state ? descriptionsOf(*attachment.definition, state, attachment.id) : JsonValue::object();
  if (attachment.definition->visibleVarName) {
    result["variableName"] = nameText(*attachment.definition->visibleVarName);
    if (auto value = attachment.variables.find(*attachment.definition->visibleVarName)) result["variableValue"] = *value;
  }
  return result;
}

JsonValue exposeEntity(const GameStatePointer& state, const EntityState& entity, bool hide) {
  static const Name ARTIFACT = intern("artifact"), WEAPON = intern("weapon"), TECHNIQUE = intern("technique");
  static const Name ABYSS = intern("abyss"), CONDUCTIVE = intern("conductive"), HINT_ICON = intern("hintIcon");
  const EntityDefinition& definition = *entity.definition;
  if (definition.id == 0) hide = true;
  JsonValue result = JsonValue::object();
  result["id"] = entity.id;
  result["type"] = hide ? 0 : exposedEntityType(definition.type);
  result["definitionId"] = hide ? 0 : definition.id;
  if (definition.visibleVarName) {
    if (auto value = entity.variables.find(*definition.visibleVarName)) result["variableValue"] = *value;
    result["variableName"] = nameText(*definition.visibleVarName);
  }
  bool hasUsagePerRound = false;
  for (Name name : usagePerRoundNames()) hasUsagePerRound = hasUsagePerRound || entity.variables.valueOr(name, 0) != 0;
  result["hasUsagePerRound"] = hasUsagePerRound;
  if (auto hintIcon = entity.variables.find(HINT_ICON)) result["hintIcon"] = *hintIcon;
  if (definition.hintText) result["hintText"] = *definition.hintText;
  if (definition.type == EntityType::Equipment) {
    result["equipment"] = definition.hasTag(ARTIFACT) ? 2 : definition.hasTag(WEAPON) ? 1 : definition.hasTag(TECHNIQUE) ? 3 : 0;
  }
  result["descriptionDictionary"] = hide || !state ? JsonValue::object() : descriptionsOf(definition, state, entity.id);
  JsonValue cost = JsonValue::array();
  if (!hide) {
    cost = exposeDiceRequirement(costOfCard(definition));
    if (cost.asArray().size() == 1 && cost.asArray()[0].at("type").asInt() == dice::Legend) {
      JsonValue aligned = JsonValue::object();
      aligned["type"] = dice::Aligned;
      aligned["count"] = 0;
      cost.asArray().insert(cost.asArray().begin(), aligned);
    }
  }
  result["definitionCost"] = std::move(cost);
  int tags = 0;
  auto addTags = [&](const std::vector<Name>& names) {
    for (Name tag : names) {
      if (tag == ABYSS) tags |= CARD_TAG_ABYSS;
      if (tag == CONDUCTIVE) tags |= CARD_TAG_CONDUCTIVE;
    }
  };
  addTags(definition.tags);
  for (const EntityPointer& attachment : entity.attachments) addTags(attachment->definition->tags);
  result["tags"] = tags;
  JsonValue attachments = JsonValue::array();
  if (!hide) {
    for (const EntityPointer& attachment : entity.attachments) attachments.push(exposeAttachment(state, *attachment));
  }
  result["attachment"] = std::move(attachments);
  return result;
}

int exposedCharacterTags(const std::vector<EntityPointer>& entities) {
  static const Name SHIELD = intern("shield"), BARRIER = intern("barrier"), DISABLE_SKILL = intern("disableSkill");
  static const Name NIGHTSOULS = intern("nightsoulsBlessing"), BOND_OF_LIFE = intern("bondOfLife");
  static const Name BARRIER_USAGE = intern("barrierUsage"), USAGE = intern("usage");
  int result = 0;
  for (const EntityPointer& entity : entities) {
    const EntityDefinition& definition = *entity->definition;
    if (definition.hasTag(SHIELD)) result |= CHARACTER_TAG_SHIELD;
    if (definition.hasTag(BARRIER) &&
        (entity->variables.valueOr(BARRIER_USAGE, 0) != 0 || entity->variables.valueOr(USAGE, 0) != 0)) {
      result |= CHARACTER_TAG_BARRIER;
    }
    if (definition.hasTag(DISABLE_SKILL)) result |= CHARACTER_TAG_DISABLE_SKILL;
    if (definition.hasTag(NIGHTSOULS)) result |= CHARACTER_TAG_NIGHTSOULS_BLESSING;
    if (definition.hasTag(BOND_OF_LIFE)) result |= CHARACTER_TAG_BOND_OF_LIFE;
  }
  return result;
}

JsonValue exposeCharacter(const GameStatePointer& state, const PlayerState& player, const CharacterState& character) {
  static const Name HEALTH = intern("health"), ENERGY = intern("energy"), MAX_HEALTH = intern("maxHealth");
  static const Name MAX_ENERGY = intern("maxEnergy"), AURA = intern("aura"), ALIVE = intern("alive");
  std::vector<EntityPointer> tagSources;
  if (player.activeCharacterId == character.id) {
    tagSources.insert(tagSources.end(), player.combatStatuses.begin(), player.combatStatuses.end());
    tagSources.insert(tagSources.end(), player.summons.begin(), player.summons.end());
  }
  tagSources.insert(tagSources.end(), character.entities.begin(), character.entities.end());
  double energy = character.variables.valueOr(ENERGY, 0);
  double maxEnergy = character.variables.valueOr(MAX_ENERGY, 0);
  const auto& specialEnergy = character.definition->specialEnergy;
  if (specialEnergy) {
    energy = character.variables.valueOr(specialEnergy->variableName, 0);
    maxEnergy = specialEnergy->slotSize;
  }
  JsonValue result = JsonValue::object();
  result["id"] = character.id;
  result["definitionId"] = character.definition->id;
  result["defeated"] = character.variables.valueOr(ALIVE, 0) == 0;
  JsonValue entities = JsonValue::array();
  for (const EntityPointer& entity : character.entities) entities.push(exposeEntity(state, *entity, false));
  result["entity"] = std::move(entities);
  result["health"] = character.variables.valueOr(HEALTH, 0);
  result["energy"] = energy;
  result["maxHealth"] = character.variables.valueOr(MAX_HEALTH, 0);
  result["maxEnergy"] = maxEnergy;
  result["aura"] = character.variables.valueOr(AURA, 0);
  result["tags"] = exposedCharacterTags(tagSources);
  if (specialEnergy) result["specialEnergyName"] = nameText(specialEnergy->variableName);
  return result;
}

JsonValue entityList(const GameStatePointer& state, const std::vector<EntityPointer>& entities, bool hide) {
  JsonValue result = JsonValue::array();
  for (const EntityPointer& entity : entities) result.push(exposeEntity(state, *entity, hide));
  return result;
}

JsonValue request(const char* kind, JsonValue value) {
  JsonValue result = JsonValue::object();
  result["$case"] = kind;
  result["value"] = std::move(value);
  return result;
}

JsonValue idArray(const std::vector<int>& ids) {
  JsonValue result = JsonValue::array();
  for (int id : ids) result.push(id);
  return result;
}

}  // namespace

JsonValue exposeEntityForMutation(const EntityState& entity, bool hide) {
  return exposeEntity({}, entity, hide);
}

JsonValue exposeState(int who, const GameStatePointer& state) {
  JsonValue result = JsonValue::object();
  result["phase"] = static_cast<int>(state->phase);
  result["currentTurn"] = state->currentTurn;
  result["roundNumber"] = state->roundNumber;
  if (state->winner) result["winner"] = *state->winner;
  JsonValue players = JsonValue::array();
  for (int index = 0; index < 2; ++index) {
    const PlayerState& player = *state->players[index];
    JsonValue exposed = JsonValue::object();
    exposed["activeCharacterId"] = player.activeCharacterId;
    exposed["pileCard"] = entityList(state, player.pile, true);
    exposed["handCard"] = entityList(state, player.hands, index != who);
    JsonValue characters = JsonValue::array();
    for (const CharacterPointer& character : player.characters) characters.push(exposeCharacter(state, player, *character));
    exposed["character"] = std::move(characters);
    JsonValue dice = JsonValue::array();
    for (DiceType type : player.dice) dice.push(index == who ? type : 0);
    exposed["dice"] = std::move(dice);
    exposed["combatStatus"] = entityList(state, player.combatStatuses, false);
    exposed["support"] = entityList(state, player.supports, false);
    exposed["summon"] = entityList(state, player.summons, false);
    JsonValue skills = JsonValue::array();
    if (index == who) {
      for (const CallerAndSkill& entry : initiativeSkillsOfPlayer(player)) {
        JsonValue skill = JsonValue::object();
        skill["definitionId"] = entry.skill->id;
        skill["definitionCost"] = exposeDiceRequirement(entry.skill->initiativeSkillConfig->requiredCost);
        skills.push(std::move(skill));
      }
    }
    exposed["initiativeSkill"] = std::move(skills);
    exposed["declaredEnd"] = player.declaredEnd;
    exposed["legendUsed"] = player.legendUsed;
    exposed["status"] = 0;
    players.push(std::move(exposed));
  }
  result["player"] = std::move(players);
  return result;
}

JsonValue exposeAction(const ActionInfo& action) {
  JsonValue value = JsonValue::object();
  const char* kind = "declareEnd";
  switch (action.type) {
    case ActionType::UseSkill: {
      kind = "useSkill";
      value["skillDefinitionId"] = action.skill->definition->id;
      JsonValue targets = JsonValue::array();
      for (const AnyState& target : action.targets) targets.push(target.id());
      value["targetIds"] = std::move(targets);
      if (action.mainDamageTargetId) value["mainDamageTargetId"] = *action.mainDamageTargetId;
      break;
    }
    case ActionType::PlayCard: {
      kind = "playCard";
      const EntityPointer& card = action.skill->caller.entity();
      value["cardId"] = card->id;
      value["cardDefinitionId"] = card->definition->id;
      JsonValue targets = JsonValue::array();
      for (const AnyState& target : action.targets) targets.push(target.id());
      value["targetIds"] = std::move(targets);
      value["willBeEffectless"] = action.willBeEffectless;
      break;
    }
    case ActionType::SwitchActive:
      kind = "switchActive";
      value["characterId"] = action.to->id;
      value["characterDefinitionId"] = action.to->definition->id;
      break;
    case ActionType::ElementalTuning:
      kind = "elementalTuning";
      value["removedCardId"] = action.card->id;
      value["targetDice"] = action.result;
      value["allowTuningAnyDice"] = action.allowTuningAnyDice;
      break;
    case ActionType::DeclareEnd: break;
  }
  JsonValue result = JsonValue::object();
  result["action"] = request(kind, std::move(value));
  result["requiredCost"] = exposeDiceRequirement(action.cost);
  JsonValue dice = JsonValue::array();
  for (DiceType type : action.autoSelectedDice) dice.push(type);
  result["autoSelectedDice"] = std::move(dice);
  result["validity"] = action.validity;
  result["preview"] = action.preview;
  result["isFast"] = action.fast;
  return result;
}

JsonValue actionRequest(const std::vector<ActionInfo>& actions) {
  JsonValue list = JsonValue::array();
  for (const ActionInfo& action : actions) list.push(exposeAction(action));
  JsonValue value = JsonValue::object();
  value["action"] = std::move(list);
  return request("action", std::move(value));
}

JsonValue chooseActiveRequest(const std::vector<int>& candidateIds) {
  JsonValue value = JsonValue::object();
  value["candidateIds"] = idArray(candidateIds);
  return request("chooseActive", std::move(value));
}

JsonValue selectCardRequest(const std::vector<int>& candidateDefinitionIds) {
  JsonValue value = JsonValue::object();
  value["candidateDefinitionIds"] = idArray(candidateDefinitionIds);
  return request("selectCard", std::move(value));
}

JsonValue rerollDiceRequest() {
  return request("rerollDice", JsonValue::object());
}

JsonValue switchHandsRequest() {
  return request("switchHands", JsonValue::object());
}

}  // namespace gitcg
