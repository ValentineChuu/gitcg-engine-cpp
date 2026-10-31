// Restart a game without redeal from its saved step-start pointer and compare every later decision and state.
#include <cstdio>
#include <stdexcept>
#include "bridge/session/seeded_shuffle.h"
#include "core/expose/expose_state.h"
#include "core/game/game.h"
#include "core/game/initial_state.h"
#include "core/json/json_text.h"
#include "definitions/attributes/lambda_slots.h"
#include "definitions/registry/registry.h"
namespace gitcg {
void registerAllDefinitions(DefinitionRegistry&);
void bindTranslatedCardCode(LambdaSlots&);
void bindExtensionHooks(LambdaSlots&);
}
using namespace gitcg;
namespace {
struct Entry { JsonValue request, state, response; };
class Players : public PlayerIo {
 public:
  Game* game = nullptr;
  GameStatePointer stepStart, saved;
  std::string savedView;
  std::vector<Entry> trace;
  const std::vector<Entry>* replay = nullptr;
  size_t position = 0;
  SeededRandom random{12345};
  JsonValue ask(int who, JsonValue request, JsonValue answer) {
    if (!saved && request.at("$case").asString() == "action") {
      saved = stepStart;
      savedView = writeJson(exposeState(0, saved));
    }
    if (!saved) return answer;
    JsonValue view = exposeState(who, game->state());
    if (replay) {
      const auto& expected = replay->at(position++);
      if (request != expected.request || view != expected.state) throw std::runtime_error("restart differs at decision " + std::to_string(position));
      return expected.response;
    }
    trace.push_back({std::move(request), std::move(view), answer});
    return answer;
  }
  ActionAnswer action(int who, const std::vector<ActionInfo>& actions) override {
    std::vector<int> valid;
    for (size_t i = 0; i < actions.size(); ++i) if (actions[i].validity == 0) valid.push_back(i);
    int index = valid.at(static_cast<size_t>(random.next() * valid.size()));
    JsonValue response = JsonValue::object(); response["index"] = index;
    auto answer = ask(who, actionRequest(actions), response);
    index = answer.at("index").asInt(); return {index, actions[index].autoSelectedDice};
  }
  int chooseActive(int who, const std::vector<int>& ids) override {
    return ask(who, chooseActiveRequest(ids), JsonValue(ids.front())).asInt();
  }
  std::vector<DiceType> rerollDice(int who) override { ask(who, rerollDiceRequest(), JsonValue()); return {}; }
  std::vector<int> switchHands(int who) override { ask(who, switchHandsRequest(), JsonValue()); return {}; }
  int selectCard(int who, const std::vector<int>& ids) override { return ask(who, selectCardRequest(ids), JsonValue(ids.front())).asInt(); }
};
}
int main(int argc, char** argv) {
  DefinitionRegistry registry; registerAllDefinitions(registry);
  bindTranslatedCardCode(*registry.lambdaSlots()); bindExtensionHooks(*registry.lambdaSlots());
  GameData data = registry.resolve("v7.1.0");
  int games = argc > 1 ? std::stoi(argv[1]) : 100;
  int identical = 0, untranslated = 0;
  size_t decisions = 0;
  for (int seed = 1; seed <= games; ++seed) {
    try {
      DeckList deck{{1301}, {}};
      Players original;
      Game game(createInitialState(data, deck, deck, seed), original); original.game = &game;
      game.onStepStart = [&](const auto& state) { original.stepStart = state; };
      game.start();
      if (!original.saved || writeJson(exposeState(0, original.saved)) != original.savedView) throw std::runtime_error("snapshot mutated");
      Players copy; copy.replay = &original.trace; copy.saved = original.saved;
      Game restarted(original.saved, copy); copy.game = &restarted;
      restarted.start();
      if (copy.position != original.trace.size() || exposeState(0, restarted.state()) != exposeState(0, game.state()))
        throw std::runtime_error("restart finished differently");
      decisions += copy.position; ++identical;
    } catch (const std::exception& error) {
      if (std::string(error.what()).find("not translated") != std::string::npos) { ++untranslated; continue; }
      std::fprintf(stderr, "seed %d: %s\n", seed, error.what()); return 1;
    }
  }
  std::printf("%d/%d unchanged-state restarts identical, %d untranslated, %zu decisions\n", identical, games, untranslated, decisions);
  return 0;
}
