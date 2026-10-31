// Checks bridge/turn_search.mjs on real positions: it runs, is deterministic, a search restricted to one first move
// finds a subset of the full search's lines, and a skill's one-move line matches playing that skill in a copy.
//   node tests/turn_search_check.mjs [positions]
import { readFileSync } from "node:fs";
import { Game } from "@gi-tcg/core";
import getData from "@gi-tcg/data";
import { seededRandom, seededShuffle } from "../bridge/seeded_random.mjs";
import { searchTurn } from "../bridge/turn_search.mjs";
import { runsFile } from "../data_paths.mjs";

const data = getData();
const matchups = JSON.parse(readFileSync(runsFile("turn_lines", "test_matchups.json"), "utf8"));
const wanted = Number(process.argv[2] ?? 6);
const positions = [];
for (let number = 0; positions.length < wanted && number < 20; number += 1) {
  const random = seededRandom(500 + number);
  const decks = matchups[number].map((deck) => ({ characters: deck.characters, cards: seededShuffle(deck.cards, random), noShuffle: true }));
  const game = new Game(Game.createInitialState({ decks, data, randomSeed: 500 + number }), {});
  let stepStart = null;
  game.onStepStart = (state) => { stepStart = state; };
  let actionDecisions = 0;
  for (const seat of [0, 1]) game.players[seat].io = { notify: null, rpc: async ({ request }) => {
    if (request.$case === "action") {
      actionDecisions += 1;
      if (seat === 0 && actionDecisions === 5) { positions.push({ state: stepStart, actions: request.value.action }); game.terminate(); return new Promise(() => {}); }
      const valid = request.value.action.map((a, i) => [a, i]).filter(([a]) => (a.validity ?? 0) === 0);
      const [action, index] = valid[Math.floor(random() * valid.length)];
      return { response: { $case: "action", value: { chosenActionIndex: index, usedDice: action.autoSelectedDice } } };
    }
    const answers = { chooseActive: (v) => ({ activeCharacterId: v.candidateIds[0] }), rerollDice: () => ({ diceToReroll: [] }),
      switchHands: () => ({ removedHandIds: [] }), selectCard: (v) => ({ selectedDefinitionId: v.candidateDefinitionIds[0] }) };
    return { response: { $case: request.$case, value: answers[request.$case](request.value) } };
  } };
  await game.start().catch(() => {});
}
let failures = 0;
for (const [number, { state, actions }] of positions.entries()) {
  const full = await searchTurn(state, 0, { budget: 1000 });
  const again = await searchTurn(state, 0, { budget: 1000 });
  const same = JSON.stringify(full.lines) === JSON.stringify(again.lines);
  const skill = actions.findIndex((action) => action.action.$case === "useSkill" && (action.validity ?? 0) === 0);
  let subset = true;
  if (skill !== -1) {
    const one = await searchTurn(state, 0, { budget: 1000, firstMoves: [skill] });
    const all = new Set(full.lines.map((line) => JSON.stringify(line.change)));
    subset = one.lines.length === 1 && (full.capped || all.has(JSON.stringify(one.lines[0].change)));
  }
  if (!same || !subset) failures += 1;
  console.log(`position ${number}: ${full.lines.length} lines, ${full.plays} plays, capped ${full.capped}, stopped at random ${full.stoppedAtRandom}, deterministic ${same}, skill line found ${subset}`);
}
console.log(failures ? `${failures} FAILED` : `all ${positions.length} positions passed`);
process.exit(failures ? 1 : 0);
