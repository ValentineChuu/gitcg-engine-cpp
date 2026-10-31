// The engine cache (core patch 7) must not change anything: the same seeded games, played with the cache on and off,
// see identical decisions (every request and the player's view) and end the same; and turn searches
// (bridge/turn_search.mjs) from the same positions find identical lines.
//   node tests/engine_cache_equivalence.mjs [games]
import { createHash } from "node:crypto";
import { readFileSync } from "node:fs";
import { exposeState, Game } from "@gi-tcg/core";
import getData from "@gi-tcg/data";
import { seededRandom, seededShuffle } from "../bridge/seeded_random.mjs";
import { searchTurn } from "../bridge/turn_search.mjs";
import { runsFile } from "../data_paths.mjs";

const data = getData();
const matchups = JSON.parse(readFileSync(runsFile("turn_lines", "test_matchups.json"), "utf8"));
const games = Number(process.argv[2] ?? 12);
const answers = { chooseActive: (v, r) => ({ activeCharacterId: v.candidateIds[Math.floor(r() * v.candidateIds.length)] }),
  rerollDice: () => ({ diceToReroll: [] }), switchHands: () => ({ removedHandIds: [] }),
  selectCard: (v, r) => ({ selectedDefinitionId: v.candidateDefinitionIds[Math.floor(r() * v.candidateDefinitionIds.length)] }) };

async function play(number, cache) {
  globalThis.autogitcgEngineCache = cache;
  const random = seededRandom(900 + number);
  const decks = matchups[number % matchups.length].map((deck) => ({ characters: deck.characters, cards: seededShuffle(deck.cards, random), noShuffle: true }));
  const game = new Game(Game.createInitialState({ decks, data, randomSeed: 900 + number }), {});
  let stepStart = null;
  game.onStepStart = (state) => { stepStart = state; };
  const trace = createHash("sha1");
  const positions = [];
  let decisions = 0;
  for (const seat of [0, 1]) game.players[seat].io = { notify: null, rpc: async ({ request }) => {
    decisions += 1;
    trace.update(JSON.stringify([seat, request, exposeState(seat, game.state)]));
    if (request.$case !== "action") return { response: { $case: request.$case, value: answers[request.$case](request.value, random) } };
    if (decisions % 37 === 0 && positions.length < 3) positions.push({ state: stepStart, who: seat, actions: request.value.action });
    const valid = request.value.action.map((a, i) => [a, i]).filter(([a]) => (a.validity ?? 0) === 0);
    const [action, index] = valid[Math.floor(random() * valid.length)];
    return { response: { $case: "action", value: { chosenActionIndex: index, usedDice: action.autoSelectedDice } } };
  } };
  let winner;
  try { winner = await game.start(); } catch (error) { winner = `error: ${error.message}`; }
  return { trace: trace.digest("hex"), winner, decisions, positions };
}

let same = 0, searchesSame = 0, searches = 0;
for (let number = 0; number < games; number += 1) {
  const off = await play(number, false), on = await play(number, true);
  if (off.trace === on.trace && off.winner === on.winner) same += 1;
  else console.log(`game ${number}: DIFFERENT (decisions ${off.decisions} vs ${on.decisions}, winner ${off.winner} vs ${on.winner})`);
  for (const position of off.positions) {
    globalThis.autogitcgEngineCache = false;
    const withoutCache = await searchTurn(position.state, position.who, { budget: 300, actions: position.actions });
    globalThis.autogitcgEngineCache = true;
    const withCache = await searchTurn(position.state, position.who, { budget: 300, actions: position.actions });
    searches += 1;
    if (JSON.stringify(withoutCache.lines) === JSON.stringify(withCache.lines)) searchesSame += 1;
  }
}
console.log(`${same}/${games} games identical with the engine cache on and off; ${searchesSame}/${searches} turn searches identical`);
process.exit(same === games && searchesSame === searches ? 0 : 1);
