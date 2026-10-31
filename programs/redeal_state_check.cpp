// Bit-exact redeal fixtures from Node, including guessed replacements, dice, both seats and random seeds.
#include <fstream>
#include <iostream>
#include "bridge/session/redeal.h"
#include "bridge/search/reply_guess.h"
#include "core/definitions/entity_definition.h"
#include "core/json/json_text.h"
#include "definitions/attributes/lambda_slots.h"
#include "definitions/registry/registry.h"
#include "programs/support/load_game_state.h"
namespace gitcg {
void registerAllDefinitions(DefinitionRegistry&);
void bindTranslatedCardCode(LambdaSlots&);
void bindExtensionHooks(LambdaSlots&);
}
using namespace gitcg;
namespace {
JsonValue entity(const EntityPointer& value) {
  JsonValue result = JsonValue::object(); result["id"] = value->id; result["definition"] = value->definition->id;
  JsonValue variables = JsonValue::object();
  for (const auto& [name, v] : value->variables.entries()) variables[nameText(name)] = v;
  result["variables"] = std::move(variables);
  JsonValue attachments = JsonValue::array();
  for (const auto& a : value->attachments) attachments.push(entity(a));
  result["attachments"] = std::move(attachments); return result;
}
JsonValue snapshot(const GameStatePointer& state) {
  JsonValue result = JsonValue::object(); result["random"] = state->randomIterator;
  JsonValue players = JsonValue::array();
  for (const auto& p : state->players) {
    JsonValue player = JsonValue::object(), hands = JsonValue::array(), pile = JsonValue::array(), dice = JsonValue::array();
    for (const auto& card : p->hands) hands.push(entity(card));
    for (const auto& card : p->pile) pile.push(entity(card));
    for (int die : p->dice) dice.push(die);
    player["hands"] = std::move(hands); player["pile"] = std::move(pile); player["dice"] = std::move(dice); players.push(std::move(player));
  }
  result["players"] = std::move(players); return result;
}
}
int main(int argc, char** argv) {
  DefinitionRegistry registry; registerAllDefinitions(registry);
  bindTranslatedCardCode(*registry.lambdaSlots()); bindExtensionHooks(*registry.lambdaSlots());
  auto data = registry.resolve("v7.1.0");
  std::ifstream input(argc > 1 ? argv[1] : "tests/generated/redeal_cases.jsonl");
  if (!input) return 1;
  int cases = 0;
  std::string line;
  while (std::getline(input, line)) {
    const auto test = parseJson(line);
    auto state = testing::loadGameState(data, test.at("state"));
    auto before = snapshot(state);
    JsonValue guessed;
    const JsonValue* cards = test.find("opponentCards");
    if (const auto* view = test.find("view")) {
      std::vector<int> played;
      for (const auto& card : test.at("played").asArray()) played.push_back(card.asInt());
      guessed = opponentCardsFor(state, test.at("who").asInt(), test.at("seed").asNumber(), *view, played);
      if (guessed != test.at("expectedCards")) throw std::logic_error("guessed cards differ in case " + std::to_string(cases));
      cards = &guessed;
    }
    try {
      auto copy = redealHiddenCards(state, test.at("who").asInt(), test.at("seed").asNumber(), cards);
      if (test.find("error") || snapshot(copy) != test.at("expected") || snapshot(state) != before)
        throw std::logic_error("redeal mismatch in case " + std::to_string(cases));
    } catch (const std::runtime_error& error) {
      const auto* expected = test.find("error");
      if (!expected || expected->asString() != error.what()) throw;
    }
    ++cases;
  }
  std::cout << cases << " redeals identical; original states unchanged\n";
}
