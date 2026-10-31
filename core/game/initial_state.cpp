#include "core/game/initial_state.h"

#include <string>

#include "core/values/engine_errors.h"

namespace gitcg {

namespace {

constexpr int INITIAL_ID = -500000;

Variables initialVariables(const std::vector<std::pair<Name, VariableConfig>>& configs) {
  Variables variables;
  for (const auto& [name, config] : configs) variables.set(name, config.initialValue);
  return variables;
}

PlayerPointer initialPlayer(int who, const GameData& data, const DeckList& deck, int& nextId, const GameConfig& config) {
  static const Name LEGEND = intern("legend");
  auto player = makeRef<PlayerState>();
  player->who = who;
  for (int id : deck.cards) {
    const EntityDefinition* definition = data.entities.find(id);
    if (!definition) throw GameDataError("Unknown card id " + std::to_string(id));
    player->initialPile.push_back(definition);
  }
  // Legend cards move to the front (only as many as the first hand holds).
  std::vector<const EntityDefinition*> pile = player->initialPile;
  int front = 0;
  for (size_t index = 0; index < pile.size() && front < config.initialHandsCount; ++index) {
    if (pile[index]->hasTag(LEGEND)) std::swap(pile[index], pile[front++]);
  }
  player->initialPile = pile;
  for (int id : deck.characters) {
    const CharacterDefinition* definition = data.characters.find(id);
    if (!definition) throw GameDataError("Unknown character id " + std::to_string(id));
    auto character = makeRef<CharacterState>();
    character->id = nextId--;
    character->definition = definition;
    character->variables = initialVariables(definition->varConfigs);
    player->characters.push_back(character);
  }
  for (const EntityDefinition* definition : pile) {
    auto card = makeRef<EntityState>();
    card->id = nextId--;
    card->definition = definition;
    card->variables = initialVariables(definition->varConfigs);
    player->pile.push_back(card);
  }
  return player;
}

}  // namespace

GameStatePointer createInitialState(const GameData& data, const DeckList& first, const DeckList& second, int64_t randomSeed) {
  auto state = makeRef<GameState>();
  state->data = &data;
  state->config.randomSeed = randomSeed;
  int nextId = INITIAL_ID;
  state->players[0] = initialPlayer(0, data, first, nextId, state->config);
  state->players[1] = initialPlayer(1, data, second, nextId, state->config);
  state->randomIterator = randomSeed;
  state->idIterator = nextId;
  for (const ExtensionDefinition* extension : data.extensions.values()) state->extensions.push_back({extension, extension->initialState});
  return state;
}

}  // namespace gitcg
