// Re-dealing hidden information in a game state, so a copy made for look-ahead search doesn't reveal what the
// searching player can't know: the searching player's own deck is reshuffled, the dice randomness gets a new seed,
// the opponent's hand and deck are dealt again (same hand size), either from their real remaining cards
// pooled (fair when their deck list is known) or from `opponentCards`, a guess of definition ids from Python,
// and the opponent's dice are rolled again (same count: only the count is visible, not the faces).
// States are immutable, so this builds new state objects (spread keeps the core's state-kind symbol key).

import { seededRandom, seededShuffle } from "./seeded_random.mjs";

export function redealHiddenCards(state, searchingPlayer, seed, opponentCards = null) {
  const random = seededRandom(seed);
  const players = state.players.map((player) => {
    if (player.who === searchingPlayer) {
      return { ...player, pile: seededShuffle(player.pile, random) };
    }
    let hidden;
    if (opponentCards) {
      // Python dealt them in order (hand first, then deck, already shuffled). Each slot keeps its place and entity
      // id and only gets a new card, so the copy's trackers (which remember hand card ids) stay consistent.
      hidden = [...player.hands, ...player.pile];
      if (opponentCards.length !== hidden.length) {
        throw new Error(`guessed ${opponentCards.length} hidden cards, the opponent has ${hidden.length}`);
      }
      hidden = hidden.map((slot, index) => guessedCard(state.data, slot, opponentCards[index]));
    } else {
      hidden = seededShuffle([...player.hands, ...player.pile], random);  // their real cards pooled (no guess)
    }
    return { ...player, hands: hidden.slice(0, player.hands.length), pile: hidden.slice(player.hands.length),
             dice: guessedDice(player, random) };
  });
  const randomSeed = Math.floor(random() * 2 ** 31);
  return { ...state, players, iterators: { ...state.iterators, random: randomSeed } };
}

// A fresh card of `definitionId` in place of a hidden card (keeping its entity id).
function guessedCard(data, slot, definitionId) {
  const definition = data.entities.get(definitionId);
  if (definition === undefined) throw new Error(`unknown guessed card ${definitionId}`);
  const variables = Object.fromEntries(Object.entries(definition.varConfigs).map(([name, { initialValue }]) => [name, initialValue]));
  return { ...slot, definition, variables, attachments: [] };
}

const OMNI = 8;
const DICE_BY_ELEMENT = { cryo: 1, hydro: 2, pyro: 3, electro: 4, anemo: 5, geo: 6, dendro: 7 };

// A guess of a player's current dice faces: each die rolled, and rerolled once unless it is omni or the element of
// one of their living characters (roughly how players reroll), sorted omni first like the engine shows them.
function guessedDice(player, random) {
  const useful = new Set(player.characters
    .filter((character) => character.variables.alive)
    .flatMap((character) => character.definition.tags.map((tag) => DICE_BY_ELEMENT[tag]).filter(Boolean)));
  const roll = () => 1 + Math.floor(random() * 8);
  return player.dice.map(() => {
    const first = roll();
    return first === OMNI || useful.has(first) ? first : roll();
  }).sort((a, b) => b - a);
}
