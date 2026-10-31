// Loader fidelity, native callbacks, builder snapshot semantics, and explicit unsupported-function failures.
#include <cmath>
#include <iostream>
#include <stdexcept>

#include "core/rules/entity_lookup.h"
#include "query/card_cost.h"
#include "query/query.h"
#include "programs/support/json_io.h"
#include "programs/support/load_definitions.h"
#include "programs/support/load_game_state.h"
#include "programs/support/verify_definitions.h"

using namespace gitcg;
using namespace gitcg::testing;
namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void variables(const Variables& actual, const JsonValue& expected) {
  require(actual.entries().size() == expected.asObject().size(), "variable count");
  for (size_t i = 0; i < actual.entries().size(); ++i) {
    require(nameText(actual.entries()[i].first) == expected.asObject()[i].first
        && actual.entries()[i].second == expected.asObject()[i].second.asNumber(), "variable order/value");
  }
}
void entity(const AnyState& actual, const JsonValue& expected) {
  require(actual.id() == expected.at("id").asInt(), "entity id");
  variables(actual.variables(), expected.at("variables"));
  if (actual.isCharacter()) {
    require(actual.character()->definition->id == expected.at("definition").asInt(), "character definition");
    const auto& entities = actual.character()->entities;
    require(entities.size() == expected.at("entities").asArray().size(), "character entities count");
    for (size_t i = 0; i < entities.size(); ++i) entity(entities[i], expected.at("entities").asArray()[i]);
  } else {
    require(actual.entity()->definition->id == expected.at("definition").asInt(), "entity definition");
    if (auto* attachments = expected.find("attachments")) {
      require(actual.entity()->attachments.size() == attachments->asArray().size(), "attachments count");
      for (size_t i = 0; i < actual.entity()->attachments.size(); ++i) entity(actual.entity()->attachments[i], attachments->asArray()[i]);
    }
  }
}
void nativeAttachmentCost() {
  SkillDefinition skill;
  skill.skillType = SkillType::PlayCard;
  InitiativeSkillConfig config;
  config.requiredCost.set(dice::Hydro, 3); config.requiredCost.set(dice::Energy, 2);
  config.requiredCost.set(dice::Legend, 1); config.diceCostSize = 3;
  skill.initiativeSkillConfig = config;
  EntityDefinition definition; definition.type = EntityType::EventCard; definition.skills = {&skill};
  auto card = makeRef<EntityState>(); card->id = 5; card->definition = &definition;
  EntityDefinition attachmentDefinition; attachmentDefinition.type = EntityType::Attachment;
  auto attachment = makeRef<EntityState>(); attachment->id = 6; attachment->definition = &attachmentDefinition;
  card->attachments.push_back(attachment);
  GameState state;
  state.players[0] = makeRef<PlayerState>(); state.players[1] = makeRef<PlayerState>();
  state.players[0]->hands.push_back(card);
  attachmentDefinition.modifications = [](const GameStatePointer& state, int id) {
    require(getEntityById(*state, id).id() == 6, "modification receives full state and attachment id");
    return std::vector<AttachmentModification>{{AttachmentModification::Kind::IncreaseCardCost, 2},
      {AttachmentModification::Kind::DecreaseCardCost, 1}, {AttachmentModification::Kind::ChangeCardCostType, 0, dice::Pyro}};
  };
  require(query::cardDiceCost(state, card) == 4, "native attachment modified cost");
  state.versionBehavior.diceCostApplyAttachments = false;
  require(query::cardDiceCost(state, card) == 3, "legacy attachment cost bypass");
  state.versionBehavior.diceCostApplyAttachments = true;
  attachmentDefinition.modifications = {};
  bool failed = false;
  try { query::cardDiceCost(state, card); } catch (const std::exception&) { failed = true; }
  require(failed, "missing modification functions must fail");
}
}  // namespace
int main(int argc, char** argv) {
  try {
    auto definitions = loadDefinitions(std::string("tests/generated/definitions_node.json"));
    verifyDefinitions(definitions);
    auto dump = readJson(argc > 1 ? argv[1] : "tests/query_generated/states.json");
    size_t count = 0, logRecords = 0;
    for (const auto& snapshot : dump.at("snapshots").asArray()) {
      const auto& source = snapshot.at("state");
      auto loaded = loadInternalState(definitions.data, source);
      const auto& state = *loaded.state;
      require(state.idIterator == source.at("iterators").at("id").asInt()
          && state.randomIterator == source.at("iterators").at("random").asNumber()
          && state.currentTurn == source.at("currentTurn").asInt()
          && state.roundNumber == source.at("roundNumber").asInt(), "root counters");
      for (int who = 0; who < 2; ++who) {
        const auto& player = *state.players[who]; const auto& input = source.at("players").asArray()[who];
        require(player.who == input.at("who").asInt() && player.activeCharacterId == input.at("activeCharacterId").asInt(), "player ids");
#define FLAG(name) require(player.name == input.at(#name).asBool(), "player flag " #name);
        FLAG(declaredEnd) FLAG(hasDefeated) FLAG(canCharged) FLAG(canPlunging) FLAG(legendUsed) FLAG(skipNextTurn) FLAG(defeatedSwitching)
#undef FLAG
#define ENTITIES(name) \
        require(player.name.size() == input.at(#name).asArray().size(), #name " count"); \
        for (size_t i = 0; i < player.name.size(); ++i) entity(player.name[i], input.at(#name).asArray()[i]);
        ENTITIES(characters) ENTITIES(hands) ENTITIES(pile) ENTITIES(summons) ENTITIES(supports) ENTITIES(combatStatuses) ENTITIES(removedEntities)
#undef ENTITIES
        require(player.initialPile.size() == input.at("initialPile").asArray().size(), "initial pile size");
        for (size_t i = 0; i < player.initialPile.size(); ++i) require(player.initialPile[i]->id == input.at("initialPile").asArray()[i].asInt(), "initial pile order");
        require(player.dice.size() == input.at("dice").asArray().size(), "dice count");
        for (size_t i = 0; i < player.dice.size(); ++i) require(player.dice[i] == input.at("dice").asArray()[i].asInt(), "dice order");
        require(player.roundSkillLog.size() == input.at("roundSkillLog").asArray().size(), "round skill log size");
        for (size_t i = 0; i < player.roundSkillLog.size(); ++i) {
          const auto& expected = input.at("roundSkillLog").asArray()[i].asArray();
          require(player.roundSkillLog[i].first == expected[0].asInt() && player.roundSkillLog[i].second.size() == expected[1].asArray().size(), "round skill log entry");
          for (size_t k = 0; k < player.roundSkillLog[i].second.size(); ++k) require(player.roundSkillLog[i].second[k] == expected[1].asArray()[k].asNumber(), "round skill order");
        }
        for (const auto* key : {"phaseDamageLog", "phaseReactionLog"}) {
          const auto& log = input.at(key);
          if (log.isArray()) logRecords += log.asArray().size();  // Shared historical arrays use $ref records.
        }
      }
      require(loaded.source == source, "retain event graphs and unrepresented configuration fields");
      ++count;
    }
    auto initial = loadGameState(definitions.data, dump.at("snapshots").asArray()[0].at("state"));
    auto all = runQuery(*initial, 0, query::dollar().character());
    auto selected = runQuery(*initial, 0, query::dollar().character().var([](const QueryVariables& v) { return v("health") >= 0; }));
    require(selected.size() == all.size(), "native variable callback");
    int calls = 0;
    require(runQuery(*initial, 0, query::dollar().any().var("missing", [&](double) { ++calls; return true; })).empty() && calls == 0,
        "missing named variables suppress the predicate");
    auto sorted = runQuery(*initial, 0, query::dollar().character().orderByFn([](const QueryVariables& v) { return -v("health"); }).limit(1));
    require(sorted.size() == 1, "native order callback");
    auto base = query::dollar().character();
    auto ordered = base.limit(1);
    auto before = stringifyJson(ordered.toExpression().expression);
    auto composite = base.union_(query::dollar().hand());
    auto compositeBefore = stringifyJson(composite.toExpression().expression);
    base.def(-1);
    require(before == stringifyJson(ordered.toExpression().expression), "ordered builders snapshot their input");
    require(compositeBefore != stringifyJson(composite.toExpression().expression), "composite builders retain operand references");
    bool failed = false;
    try { runQuery(*initial, 0, query::list({"variables", query::list({"fn", "v => { while (true) {} }"})})); }
    catch (const std::exception&) { failed = true; }
    require(failed, "unsupported serialized JS must fail explicitly");
    nativeAttachmentCost();
    std::cout << "Static definitions: " << definitions.characters.size() << " characters, " << definitions.entities.size()
        << " entities, " << definitions.attachments.size() << " attachments, " << definitions.extensions.size()
        << " extensions, " << definitions.skills.size() << " skills: every dumped field verified\n"
        << count << " loaded states verified; " << logRecords << " event log records retained in fixture JSON\n"
        << "Native callbacks, builder references/snapshots, attachment callbacks, and explicit failures: pass\n";
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
