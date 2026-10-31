// Deterministic counterpart of reference/benchmark_throughput.mjs: the same NPC deck table, no previews/notifications,
// fixed deck order on both backends, independent per-game random streams so untranslated prefixes do not skew
// the next game's pairing. C++: benchmark_random_games COUNT DECKS OFFSET.
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { Game } from '@gi-tcg/core';
import getData from '@gi-tcg/data';
import { seededRandom } from '../reference/bridge/seeded_random.mjs';
import { NPC_DECK_TABLE } from "../reference/data_paths.mjs";
export async function benchmarkGames(count, offset) {
  const data = getData();
  const decks = JSON.parse(readFileSync(NPC_DECK_TABLE))
    .filter(d => d.characterList?.length === 3 && d.cardList?.length === 30)
    .map(d => ({ characters: d.characterList, cards: d.cardList, noShuffle: true }));
  let decisions = 0, finished = 0, rounds = 0, errors = 0;
  const started = performance.now();
  for (let index = 0; index < count; ++index) {
    const random = seededRandom(12345 + offset + index), pick = items => items[Math.floor(random() * items.length)];
    const pair = [pick(decks), pick(decks)];
    const game = new Game(Game.createInitialState({ data, decks: pair, randomSeed: Math.floor(random() * 2 ** 31) }));
    const io = { notify: null, rpc: async ({ request }) => {
      ++decisions;
      let value;
      switch (request.$case) {
        case 'chooseActive': value = { activeCharacterId: pick(request.value.candidateIds) }; break;
        case 'selectCard': value = { selectedDefinitionId: pick(request.value.candidateDefinitionIds) }; break;
        case 'rerollDice': value = { diceToReroll: [] }; break;
        case 'switchHands': value = { removedHandIds: [] }; break;
        case 'action': {
          const [action, i] = pick(request.value.action.map((a, i) => [a, i]).filter(([a]) => a.validity === 0));
          value = { chosenActionIndex: i, usedDice: action.autoSelectedDice }; break;
        }
      }
      return { response: { $case: request.$case, value } };
    } };
    for (const player of game.players) player.io = io;
    try { await game.start(); finished++; rounds += game.state.roundNumber; }
    catch { errors++; }
  }
  const seconds = (performance.now() - started) / 1000;
  return { backend: 'node', attempted: count, finished, untranslated: 0, different: errors, decisions, rounds, seconds,
    gamesPerSecond: finished / seconds, decisionsPerSecond: decisions / seconds };
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  console.log(JSON.stringify(await benchmarkGames(Number(process.argv[2] ?? 60), Number(process.argv[3] ?? 0))));
}
