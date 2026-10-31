// Query fixtures use raw internal states, never exposed player views. Variables retain source order.
#include "programs/support/load_game_state.h"

#include <stdexcept>

namespace gitcg::testing {
namespace {
Variables variables(const JsonValue& source) {
  Variables result;
  for (const auto& [key, value] : source.asObject()) result.set(intern(key), value.asNumber());
  return result;
}
EntityPointer entity(const GameData& data, const JsonValue& source) {
  auto result = makeRef<EntityState>();
  result->id = source.at("id").asInt();
  int definition = source.at("definition").asInt();
  bool attachment = source.at("type").asString() == "attachment";
  result->definition = attachment ? data.attachments.find(definition) : data.entities.find(definition);
  if (!result->definition) throw std::runtime_error("Missing entity definition " + std::to_string(definition));
  result->variables = variables(source.at("variables"));
  if (auto* attachments = source.find("attachments")) {
    for (const auto& value : attachments->asArray()) result->attachments.push_back(entity(data, value));
  }
  return result;
}
CharacterPointer character(const GameData& data, const JsonValue& source) {
  auto result = makeRef<CharacterState>();
  result->id = source.at("id").asInt();
  result->definition = data.characters.find(source.at("definition").asInt());
  if (!result->definition) throw std::runtime_error("Missing character definition");
  result->variables = variables(source.at("variables"));
  for (const auto& value : source.at("entities").asArray()) result->entities.push_back(entity(data, value));
  return result;
}
Phase phase(const std::string& text) {
  if (text == "initHands") return Phase::InitHands;
  if (text == "initActives") return Phase::InitActives;
  if (text == "roll") return Phase::Roll;
  if (text == "action") return Phase::Action;
  if (text == "end") return Phase::End;
  if (text == "gameEnd") return Phase::GameEnd;
  throw std::runtime_error("Unknown phase " + text);
}
}  // namespace
GameStatePointer loadGameState(const GameData& data, const JsonValue& source) {
  auto result = makeRef<GameState>();
  result->data = &data;
  const auto& config = source.at("config");
  result->config.randomSeed = static_cast<int64_t>(config.at("randomSeed").asNumber());
#define CONFIG(name) result->config.name = config.at(#name).asInt();
  CONFIG(initialHandsCount) CONFIG(maxHandsCount) CONFIG(maxPileCount) CONFIG(maxRoundsCount)
  CONFIG(maxSupportsCount) CONFIG(maxSummonsCount) CONFIG(initialDiceCount) CONFIG(maxDiceCount)
#undef CONFIG
  const auto& behavior = source.at("versionBehavior");
  result->versionBehavior.recreateTakesMax = behavior.at("defaultRecreateBehavior").asString() == "takeMax";
  result->versionBehavior.foodOmitInjuredOnly = behavior.at("foodOmitInjuredOnly").asBool();
  result->versionBehavior.discardMaxCostHandsAbortPreview = behavior.at("discardMaxCostHandsAbortPreview").asBool();
  result->versionBehavior.diceCostApplyAttachments = behavior.at("diceCostApplyAttachments").asBool();
  result->randomIterator = static_cast<int64_t>(source.at("iterators").at("random").asNumber());
  result->idIterator = source.at("iterators").at("id").asInt();
  result->phase = phase(source.at("phase").asString());
  if (!source.at("prevPhase").isNull()) result->previousPhase = phase(source.at("prevPhase").asString());
  result->roundNumber = source.at("roundNumber").asInt();
  result->currentTurn = source.at("currentTurn").asInt();
  if (!source.at("winner").isNull()) result->winner = source.at("winner").asInt();
  for (int who = 0; who < 2; ++who) {
    const auto& player = source.at("players").asArray().at(who);
    auto target = makeRef<PlayerState>();
    target->who = player.at("who").asInt();
    for (const auto& id : player.at("initialPile").asArray()) {
      auto* definition = data.entities.find(id.asInt());
      if (!definition) throw std::runtime_error("Missing initial pile definition");
      target->initialPile.push_back(definition);
    }
    target->activeCharacterId = player.at("activeCharacterId").asInt();
    for (const auto& value : player.at("characters").asArray()) target->characters.push_back(character(data, value));
#define ENTITIES(name) for (const auto& value : player.at(#name).asArray()) target->name.push_back(entity(data, value));
    ENTITIES(pile) ENTITIES(hands) ENTITIES(combatStatuses) ENTITIES(supports) ENTITIES(summons)
#undef ENTITIES
    for (const auto& die : player.at("dice").asArray()) target->dice.push_back(die.asInt());
#define FLAG(name) target->name = player.at(#name).asBool();
    FLAG(declaredEnd) FLAG(hasDefeated) FLAG(canCharged) FLAG(canPlunging) FLAG(legendUsed) FLAG(skipNextTurn) FLAG(defeatedSwitching)
#undef FLAG
    for (const auto& entry : player.at("roundSkillLog").asArray()) {
      std::vector<double> skillIds;
      for (const auto& id : entry.asArray().at(1).asArray()) skillIds.push_back(id.asNumber());
      target->roundSkillLog.emplace_back(entry.asArray().at(0).asInt(), std::move(skillIds));
    }
    for (const auto& value : player.at("removedEntities").asArray()) {
      if (value.at("type").asString() == "character") target->removedEntities.emplace_back(character(data, value));
      else target->removedEntities.emplace_back(entity(data, value));
    }
    result->players[who] = target;
  }
  for (const auto& sourceExtension : source.at("extensions").asArray()) {
    const auto* definition = data.extensions.find(sourceExtension.at("definition").asInt());
    if (!definition) throw std::runtime_error("Missing extension definition");
    result->extensions.push_back({definition, sourceExtension.at("state")});
  }
  return result;
}
}  // namespace gitcg::testing
