// Independent oracle fixtures for the bridge LCG, shuffle, guessed card replacement and useful-die rerolls.
import { writeFileSync } from 'node:fs';
import { Game, StateSymbol } from '@gi-tcg/core';
import getData from '@gi-tcg/data';
import { redealHiddenCards } from '../reference/bridge/redeal.mjs';
import { opponentCardsFor } from '../reference/bridge/opponent_replies.mjs';
import { snapshotState } from './state_snapshot.mjs';
const data = getData();
const cards = [332004, 333003, 312004, 321001, 332001, 330001, 332006];
function hidden(state) {
  const card = c => ({ id: c.id, definition: c.definition.id, variables: { ...c.variables }, attachments: (c.attachments ?? []).map(card) });
  return { random: state.iterators.random, players: state.players.map(p => ({ hands: p.hands.map(card), pile: p.pile.map(card), dice: [...p.dice] })) };
}
const lines = [];
for (let index = 0; index < 100; ++index) {
  const game = new Game(Game.createInitialState({ data, randomSeed: index,
    decks: [0, 1].map(() => ({ characters: [1101, 1201, 1301], cards: index % 7 === 0 ? [] : cards, noShuffle: true })) }));
  for (const who of [0, 1]) {
    for (const card of game.state.players[who].pile.slice(0, index % 6))
      game.mutate({ type: 'moveEntity', from: { type: 'pile', who }, target: { type: 'hands', who }, value: card, reason: 'draw' });
    game.mutate({ type: 'resetDice', who, value: Array.from({ length: index % 17 }, (_, i) => i % 8 + 1), reason: 'roll' });
    const host = game.state.players[who].hands[0] ?? game.state.players[who].pile[0];
    if (host) {
      const definition = data.attachments.get(202);
      game.mutate({ type: 'createAttachment', target: { type: game.state.players[who].hands.length ? 'hands' : 'pile', who, cardId: host.id },
        value: { [StateSymbol]: 'attachment', id: 0, definition, variables: { layer: 3 } } });
    }
    if (index % 3 === 0) game.mutate({ type: 'modifyEntityVar', state: game.state.players[who].characters[0], varName: 'alive', value: 0, oldValue: 1, direction: null });
  }
  for (const who of [0, 1]) for (const seed of [0, 1, 2147483647, 4294967295, 9007199254740991, index * 7919]) {
    const opponent = game.state.players[1 - who];
    const count = opponent.hands.length + opponent.pile.length;
    const guesses = [null, Array.from({ length: count }, (_, i) => cards[(i + index) % cards.length]), [999999], Array.from({ length: count }, () => 999999)];
    for (const opponentCards of guesses) {
      const entry = { state: snapshotState(game.state), who, seed, opponentCards };
      try { entry.expected = hidden(redealHiddenCards(game.state, who, seed, opponentCards)); }
      catch (error) { entry.error = error.message; }
      lines.push(JSON.stringify(entry));
    }
    if (seed === 0) {
      const candidates = Array.from({ length: 12 }, (_, i) => [(i % 3) - 1,
        Array.from({ length: 3 + i % 2 }, (_, j) => cards[(i + j) % cards.length])]);
      const played = [cards[index % cards.length], cards[index % cards.length], cards[(index + 3) % cards.length]];
      for (const view of [{ opponentDeck: 'guessed', candidates }, { opponentDeck: 'guessed', candidates: [] },
                          { opponentDeck: 'hidden' }, { opponentDeck: 'open' }]) {
        const entry = { state: snapshotState(game.state), who, seed: index * 7919 + 123, view, played };
        entry.expectedCards = opponentCardsFor(game.state, who, entry.seed, view, played);
        entry.expected = hidden(redealHiddenCards(game.state, who, entry.seed, entry.expectedCards));
        lines.push(JSON.stringify(entry));
      }

    }
  }
}
writeFileSync('tests/generated/redeal_cases.jsonl', lines.join('\n') + '\n');
console.log(`${lines.length} redeal fixtures`);
