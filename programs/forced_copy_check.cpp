// The constructor's forced-first-answer contract: a changed action list must fail once, with no decision emitted.
#include <cstdio>
#include "bridge/session/game_session.h"
#include "core/game/initial_state.h"
#include "definitions/attributes/lambda_slots.h"
#include "definitions/registry/registry.h"
namespace gitcg {
void registerAllDefinitions(DefinitionRegistry&);
void bindTranslatedCardCode(LambdaSlots&);
void bindExtensionHooks(LambdaSlots&);
}
using namespace gitcg;
int main() {
  DefinitionRegistry registry; registerAllDefinitions(registry);
  bindTranslatedCardCode(*registry.lambdaSlots()); bindExtensionHooks(*registry.lambdaSlots());
  auto data = registry.resolve("v7.1.0"); DeckList deck{{1301}, {}};
  auto state = createInitialState(data, deck, deck, 1);
  auto& root = editable(state); root.phase = Phase::Action; root.previousPhase = Phase::Roll; root.roundNumber = 1;
  for (auto& p : root.players) { auto& player = editable(p); player.activeCharacterId = player.characters[0]->id; player.dice.assign(8, 8); }
  JsonValue response = JsonValue::object(); response["$case"] = "action";
  JsonValue value = JsonValue::object(); value["chosenActionIndex"] = 0; value["usedDice"] = JsonValue::array(); response["value"] = value;
  Scheduler scheduler; std::vector<JsonValue> messages;
  GameSession session(99, state, scheduler, [&](const JsonValue& m) { messages.push_back(m); },
    [&](GameSession&, int) { throw std::logic_error("divergent copy asked a decision"); },
    GameSession::ForcedAnswer{0, JsonValue::array(), response});
  session.start(); scheduler.runUntilIdle();
  if (!session.done() || messages.size() != 1 || messages[0].at("type").asString() != "failed" ||
      messages[0].at("message").asString() != "copy diverged: its first decision differs from the original") return 1;
  std::puts("forced first answer: divergent copies fail once and stop");
}
