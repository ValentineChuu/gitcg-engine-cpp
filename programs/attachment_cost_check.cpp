// Conductive has triggered skills but no cost modifiers: queries must treat its executable modifier list as empty.
#include <cstdio>
#include <stdexcept>
#include "core/definitions/game_data.h"
#include "core/rules/costs.h"
#include "query/card_cost.h"
#include "definitions/registry/registry.h"

namespace gitcg {
void registerAllDefinitions(DefinitionRegistry&);
void bindTranslatedCardCode(LambdaSlots&);
}
using namespace gitcg;

int main() {
  DefinitionRegistry registry;
  registerAllDefinitions(registry);
  bindTranslatedCardCode(*registry.lambdaSlots());
  GameData data = registry.resolve("v7.1.0");
  GameState state;
  state.data = &data;
  state.players[0] = makeRef<PlayerState>();
  state.players[1] = makeRef<PlayerState>();
  auto card = makeRef<EntityState>();
  card->id = 5;
  card->definition = data.entities.find(332004);  // Strategize: one die.
  state.players[0]->hands.push_back(card);
  auto conductive = makeRef<EntityState>();
  conductive->id = 6;
  conductive->definition = data.attachments.find(204);
  conductive->variables.set(intern("layer"), 1);
  card->attachments.push_back(conductive);
  if (!conductive->definition->modifications || !conductive->definition->modifications(makeRef<GameState>(state), 6).empty())
    throw std::runtime_error("Conductive must expose an empty executable modifier list");
  if (query::cardDiceCost(state, card) != 1) throw std::runtime_error("Conductive changed the queried card cost");
  auto reduction = makeRef<EntityState>();
  reduction->id = 7;
  reduction->definition = data.attachments.find(202);
  reduction->variables.set(intern("layer"), 1);
  card->attachments.push_back(reduction);
  if (query::cardDiceCost(state, card) != 0 || diceCostSizeOfCard(makeRef<GameState>(state), *card) != 0)
    throw std::runtime_error("A modifier after Conductive was skipped");
  std::puts("Conductive: empty modifier list; queried cost 1; mixed with Cost Reduction: queried/core cost 0");
}
