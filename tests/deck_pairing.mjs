// Picks the two decks of a test game from the exported deck sources (export_test_decks.py).
// Player 0 always has a tournament deck unless the game is NPC vs NPC; player 1's deck comes from the chosen source.
// Stage decks bring their equipment (Serious Showdown talents), which the stages define for player 1.

function pick(items, random) {
  return items[Math.floor(random() * items.length)];
}

const decksBySource = new WeakMap();

function sourcesOf(decks) {
  if (!decksBySource.has(decks)) {
    const bySource = {};
    for (const deck of decks) (bySource[deck.origin.split(":")[0]] ??= []).push(deck);
    decksBySource.set(decks, bySource);
  }
  return decksBySource.get(decks);
}

export function pairDecks(decks, sources, random) {
  const bySource = sourcesOf(decks);
  const source = pick(sources, random);
  const second = pick(bySource[source], random);
  const first = pick(bySource[source === "npc" ? "npc" : "tournament"], random);
  return {
    decks: [first, second].map((deck) => ({ characters: deck.characters, cards: deck.cards })),
    equipment: second.equipment,
    origins: [first.origin, second.origin],
  };
}
