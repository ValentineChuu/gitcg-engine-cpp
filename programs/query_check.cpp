// Compare JSON S-expressions and independently replayed C++ builder queries against the Node oracle.
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

#include "query/runtime.h"
#include "programs/support/json_io.h"
#include "programs/support/load_definitions.h"
#include "programs/support/load_game_state.h"
#include "programs/support/query_recipe.h"
#include "programs/support/verify_definitions.h"

using namespace gitcg;
using namespace gitcg::testing;
namespace {
JsonValue ids(const std::vector<AnyState>& result) {
  JsonValue::Array value;
  value.reserve(result.size());
  for (const auto& entity : result) value.emplace_back(entity.id());
  return value;
}
}  // namespace
int main(int argc, char** argv) {
  size_t checked = 0, builders = 0, expectedFailures = 0;
  std::string context;
  try {
    std::string prefix = "tests/";
    auto definitions = loadDefinitions(argc > 1 ? std::string(argv[1]) : prefix + "generated/definitions_node.json");
    verifyDefinitions(definitions);
    auto snapshots = readJson(argc > 2 ? argv[2] : prefix + "query_generated/states.json");
    auto edges = readJson(argc > 4 ? argv[4] : prefix + "query_generated/edge_states.json");
    std::vector<GameStatePointer> states;
    for (const auto* source : {&snapshots, &edges}) for (const auto& entry : source->at("snapshots").asArray()) {
      states.push_back(loadGameState(definitions.data, entry.at("state")));
    }
    std::ifstream cases(argc > 3 ? argv[3] : prefix + "query_generated/cases.jsonl");
    if (!cases) throw std::runtime_error("Cannot open case file");
    std::set<int> seenStates;
    size_t declaredCases = 0;
    std::string line;
    while (std::getline(cases, line)) {
      if (line.empty()) continue;
      auto entry = parseJson(line);
      if (auto* kind = entry.find("kind")) {
        if (kind->asString() == "summary") declaredCases = entry.at("cases").asNumber();
        else if (kind->asString() == "metadata" && entry.at("states").asInt() + entry.at("edgeStates").asInt() != static_cast<int>(states.size())) {
          throw std::runtime_error("Case metadata and state files disagree");
        }
        continue;
      }
      int index = entry.at("state").asInt(), who = entry.at("who").asInt();
      const auto& expression = entry.at("expression");
      context = "state=" + std::to_string(index) + " who=" + std::to_string(who) + " name=" + entry.at("name").asString()
          + "\nS-expression: " + stringifyJson(expression);
      bool expectsError = entry.find("expectedError");
      if (expectsError) {
        bool threw = false;
        try { runQuery(*states.at(index), who, expression); } catch (const std::exception&) { threw = true; }
        if (!threw) throw std::runtime_error("Expected JavaScript error: " + entry.at("expectedError").asString());
        if (auto* recipe = entry.find("recipe")) {
          auto built = buildQueryRecipe(*recipe);
          threw = false;
          try { runQuery(*states.at(index), who, built); } catch (const std::exception&) { threw = true; }
          if (!threw) throw std::runtime_error("Builder did not reproduce JavaScript error");
          ++builders;
        }
        seenStates.insert(index); ++checked; ++expectedFailures;
        continue;
      }
      auto got = ids(runQuery(*states.at(index), who, expression));
      if (!(got == entry.at("expected"))) {
        throw std::runtime_error("Runtime mismatch\nexpected ids: " + stringifyJson(entry.at("expected")) + "\ngot ids:      " + stringifyJson(got));
      }
      if (auto* recipe = entry.find("recipe")) {
        auto built = buildQueryRecipe(*recipe);
        auto builtExpression = built.toExpression();
        if (!recipeHasFunction(*recipe) && stringifyJson(builtExpression.expression) != stringifyJson(expression)) {
          throw std::runtime_error("Builder expression mismatch\nC++ expression: " + stringifyJson(builtExpression.expression));
        }
        auto builtIds = ids(runQuery(*states.at(index), who, builtExpression));
        if (!(builtIds == entry.at("expected"))) throw std::runtime_error("Builder result mismatch\nexpected ids: "
            + stringifyJson(entry.at("expected")) + "\ngot ids:      " + stringifyJson(builtIds));
        ++builders;
      }
      seenStates.insert(index); ++checked;
    }
    if (!checked || seenStates.size() != states.size() || checked != declaredCases) throw std::runtime_error("Empty or incomplete case coverage");
    std::cout << seenStates.size() << " states, " << checked << " JSON queries, " << builders
              << " C++ builder queries: all ids and order match (" << expectedFailures << " expected JS errors reproduced)\n";
  } catch (const std::exception& error) {
    std::cerr << context << "\n" << error.what() << "\nAfter " << checked << " JSON and " << builders << " builder cases\n";
    return 1;
  }
}
