#include "bridge/session/redeal.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "bridge/session/seeded_shuffle.h"
#include "core/definitions/game_data.h"
#include "core/rules/character_traits.h"

namespace gitcg {

GameStatePointer redealHiddenCards(const GameStatePointer& state, int searchingPlayer, double seed,
                                  const JsonValue* opponentCards) {
  SeededRandom random(seed);
  GameStatePointer result = state;
  GameState& root = editable(result);
  for (int who : {0, 1}) {
    PlayerState& player = editable(root.players[who]);
    if (who == searchingPlayer) {
      player.pile = seededShuffle(player.pile, random);
      continue;
    }
    std::vector<EntityPointer> hidden = player.hands;
    hidden.insert(hidden.end(), player.pile.begin(), player.pile.end());
    if (opponentCards && !opponentCards->isNull()) {
      const auto& cards = opponentCards->asArray();
      if (cards.size() != hidden.size()) {
        throw std::runtime_error("guessed " + std::to_string(cards.size()) + " hidden cards, the opponent has " +
                                 std::to_string(hidden.size()));
      }
      for (size_t index = 0; index < hidden.size(); ++index) {
        int id = cards[index].asInt();
        const EntityDefinition* definition = state->data->entities.find(id);
        if (!definition) throw std::runtime_error("unknown guessed card " + std::to_string(id));
        EntityState& slot = editable(hidden[index]);
        slot.definition = definition;
        slot.variables = {};
        for (const auto& [name, config] : definition->varConfigs) slot.variables.set(name, config.initialValue);
        slot.attachments.clear();
      }
    } else {
      hidden = seededShuffle(std::move(hidden), random);
    }
    size_t handCount = player.hands.size();
    player.hands.assign(hidden.begin(), hidden.begin() + handCount);
    player.pile.assign(hidden.begin() + handCount, hidden.end());
    const std::set<DiceType> useful = playerUsefulDice(player);
    auto roll = [&] { return 1 + static_cast<int>(std::floor(random.next() * 8)); };
    for (DiceType& die : player.dice) {
      int first = roll();
      die = first == dice::Omni || useful.contains(first) ? first : roll();
    }
    std::sort(player.dice.begin(), player.dice.end(), std::greater<>());
  }
  root.randomIterator = static_cast<int64_t>(std::floor(random.next() * 2147483648.0));
  return result;
}

}  // namespace gitcg
