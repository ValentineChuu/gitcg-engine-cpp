// How many distinct positions can a player reach with their own moves over the next 1, 2 and 3 of their turns, if the
// opponent only passes? (to size an input that plays out own setups). Moves that are
// invalid or would do nothing (willBeEffectless) are trimmed; lines reaching the same position are merged (so two
// orders of the same moves count once). Rerolls keep their dice, other choices take the first option, so after a round
// ends it's one sample of the next round. Positions come from games on real tournament decks with a rough scripted
// player, sampled at the start of seat 0's turns in rounds 1-4.
// Each position is counted twice on the same lines: trimmed by the engine's own flags plus one tuning per target die,
// and with the line rules on top (turn_line_rules.mjs: useless food, duplicate copies; draw and random cards end the
// line). One engine play per move: the move is played and the next move list read in the same play. Each play is timed
// (setting up the copy, then running the move to the next decision).
//   node tools/count_own_turn_lines.mjs ../runs/turn_lines/test_matchups.json <games> [cap] [first game]

import { createHash } from "node:crypto";
import { readFileSync } from "node:fs";
import { exposeState, Game } from "@gi-tcg/core";
import getData from "@gi-tcg/data";
import { performance } from "node:perf_hooks";
import { seededRandom, seededShuffle } from "../bridge/seeded_random.mjs";
import { branchingMoves, endsLine } from "../bridge/turn_line_rules.mjs";

const data = getData();
const [matchupsFile, gameCount = "40", capArgument = "3000", firstGame = "0"] = process.argv.slice(2);
const matchups = JSON.parse(readFileSync(matchupsFile, "utf8"));
const CAP = Number(capArgument);
const MAX_OPEN = 2000;  // open positions kept per depth; beyond that the count is capped (memory: each is a full game state)
const never = () => new Promise(() => {});  // a fresh one per stopped copy, so the copy can be freed

function answer(request, chooseAction) {
  const value = request.value;
  switch (request.$case) {
    case "chooseActive": return { $case: "chooseActive", value: { activeCharacterId: value.candidateIds[0] } };
    case "rerollDice": return { $case: "rerollDice", value: { diceToReroll: [] } };
    case "switchHands": return { $case: "switchHands", value: { removedHandIds: [] } };
    case "selectCard": return { $case: "selectCard", value: { selectedDefinitionId: value.candidateDefinitionIds[0] } };
    case "action": {
      const index = chooseAction(value.action);
      return { $case: "action", value: { chosenActionIndex: index, usedDice: value.action[index].autoSelectedDice } };
    }
  }
}

const passIndex = (actions) => actions.findIndex((action) => action.action.$case === "declareEnd" && (action.validity ?? 0) === 0);
const useful = (action) => (action.validity ?? 0) === 0 && !action.action.value?.willBeEffectless;

const timing = { plays: 0, setupMs: 0, runMs: 0 };

function fingerprint(state, who) {
  const strip = (value) => Array.isArray(value) ? value.map(strip)
    : value && typeof value === "object" ? Object.fromEntries(Object.entries(value).filter(([key]) => key !== "id" && key !== "cardId")
      .map(([key, inner]) => [key, strip(inner)])) : value;
  const view = strip(exposeState(who, state));
  for (const player of view.player) {
    player.handCard?.sort((a, b) => (a.definitionId ?? 0) - (b.definitionId ?? 0));
  }
  return createHash("sha1").update(JSON.stringify(view)).digest("base64");
}

// Plays from `state` (`who`'s action decision at a step start): answers that decision with `index` (or, without one,
// only reads the offered actions: the starting position), the opponent always passes, and stops at `who`'s next action
// decision, returning its step-start state and the moves offered there.
function playOne(state, who, index) {
  const setupStarted = performance.now();
  const game = new Game(state, {});
  const runStarted = performance.now();
  timing.plays += 1;
  timing.setupMs += runStarted - setupStarted;
  let stepStart = null;
  game.onStepStart = (next) => { stepStart = next; };
  return new Promise((resolve) => {
    let answeredFirst = false;
    let turnsEnded = 0;
    const finish = (result) => { timing.runMs += performance.now() - runStarted; resolve(result); game.terminate(); };
    for (const player of [0, 1]) {
      game.players[player].io = {
        notify: null,
        rpc: ({ request }) => {
          if (player === who && request.$case === "action") {
            if (index === undefined) { finish({ state, actions: request.value.action }); return never(); }
            if (answeredFirst) { finish({ state: stepStart, actions: request.value.action, turnsEnded }); return never(); }
            answeredFirst = true;
            const chosen = request.value.action[index];
            if (!chosen.isFast) turnsEnded += 1;  // a skill, a switch or ending the round ends my turn
            return Promise.resolve({ response: answer(request, () => index) });
          }
          return Promise.resolve({ response: answer(request, (actions) => {
            const pass = passIndex(actions);
            return pass === -1 ? actions.findIndex((action) => (action.validity ?? 0) === 0) : pass;
          }) });
        },
      };
    }
    game.start().then(() => finish({ ended: true, turnsEnded }), () => finish({ ended: true, turnsEnded }));
  });
}

async function countLines(start, who, TURNS, useRules) {
  const reached = [new Set(), new Set(), new Set(), new Set()];  // by own turns completed
  const root = await playOne(start, who);
  let frontier = [{ state: start, turns: 0, actions: root.actions }];
  let games = 1;
  let capped = false;
  let stoppedAtRandom = 0;
  const layers = [];  // distinct positions still open (mid-turn or later) after each move depth
  while (frontier.length && !capped) {
    layers.push(frontier.length);
    const next = [];
    for (const node of frontier) {
      for (const index of branchingMoves(node.actions, exposeState(who, node.state), useRules)) {
        if (games >= CAP) { capped = true; break; }
        const result = await playOne(node.state, who, index);
        games += 1;
        const turns = Math.min(node.turns + result.turnsEnded, TURNS);
        if (result.ended || !result.state) {
          for (let level = Math.max(turns, 1); level <= TURNS; level += 1) reached[level].add(`ended:${games}`);
          continue;
        }
        const key = fingerprint(result.state, who);
        if (useRules && endsLine(node.actions[index])) {  // drawn cards or random results: the line stops here
          stoppedAtRandom += 1;
          for (let level = Math.max(turns, 1); level <= TURNS; level += 1) reached[level].add(`stopped:${key}`);
          continue;
        }
        if (turns >= 1 && turns > node.turns) reached[turns].add(key);
        if (turns < TURNS && next.length >= MAX_OPEN) { capped = true; break; }
        if (turns < TURNS && !reached[0].has(`${turns}:${key}`)) {
          reached[0].add(`${turns}:${key}`);
          next.push({ state: result.state, turns, actions: result.actions });
        }
      }
      if (capped) break;
    }
    frontier = next;
  }
  return { distinct: reached[TURNS].size, games, capped, layers, stoppedAtRandom };
}

// Depths 1 and 2, each on its own budget, with the engine's flags only and with the line rules.
async function countByDepth(start, who) {
  const counted = {};
  for (const mode of ["flags", "rules"]) {
    counted[mode] = [];
    for (const turns of [1, 2]) {
      const result = await countLines(start, who, turns, mode === "rules");
      counted[mode].push(result);
      if (result.capped) break;
    }
  }
  return counted;
}

async function samplePositions(decks, seed) {
  const random = seededRandom(seed);
  const shuffled = decks.map((deck) => ({ characters: deck.characters, cards: seededShuffle(deck.cards, random), noShuffle: true }));
  const game = new Game(Game.createInitialState({ decks: shuffled, data, randomSeed: seed }), {});
  let stepStart = null;
  game.onStepStart = (state) => { stepStart = state; };
  const positions = [];
  let turnStart = [true, true];
  for (const player of [0, 1]) {
    game.players[player].io = {
      notify: null,
      rpc: async ({ request }) => {
        if (request.$case === "action" && player === 0 && turnStart[0] && game.state.roundNumber <= 4 && random() < 0.5) {
          positions.push({ round: game.state.roundNumber, depths: await countByDepth(stepStart, 0),
                           options: branchingMoves(request.value.action, exposeState(0, game.state), true).length });
        }
        const response = answer(request, (actions) => {  // rough player: burst, else a skill, else a card or tuning, else end
          const valid = actions.map((action, index) => [action, index]).filter(([action]) => (action.validity ?? 0) === 0);
          const skills = valid.filter(([action]) => action.action.$case === "useSkill");
          const cards = valid.filter(([action]) => action.action.$case === "playCard");
          const pool = skills.length && random() < 0.7 ? skills : cards.length && random() < 0.6 ? cards : valid;
          return pool.length ? pool[Math.floor(random() * pool.length)][1] : passIndex(actions);
        });
        if (request.$case === "action") turnStart[player] = !request.value.action[response.value.chosenActionIndex].isFast;
        return { response };
      },
    };
  }
  try { await game.start(); } catch { /* a scripted game that breaks only loses its later samples */ }
  return positions;
}

const all = [];
for (let number = Number(firstGame); number < Number(firstGame) + Number(gameCount); number += 1) {
  const started = Date.now();
  const positions = await samplePositions(matchups[number % matchups.length], 1000 + number);
  for (const position of positions) position.seconds = (Date.now() - started) / 1000 / positions.length;
  all.push(...positions);
  process.stderr.write(`game ${number + 1}: ${positions.length} positions\n`);
}
const median = (values) => { const sorted = [...values].sort((a, b) => a - b); return sorted[Math.floor(sorted.length / 2)]; };
const mean = (values) => values.reduce((a, b) => a + b, 0) / Math.max(values.length, 1);
const describe = (mode, level) => {
  const done = all.filter((p) => p.depths[mode][level] && !p.depths[mode][level].capped);
  const values = done.map((p) => p.depths[mode][level].distinct);
  const sorted = [...values].sort((a, b) => a - b);
  return { turns: level + 1, counted: done.length, overBudget: all.length - done.length, distinctMedian: median(values),
           distinct90th: sorted[Math.floor(sorted.length * 0.9)], distinctMax: sorted.at(-1), playsMedian: median(done.map((p) => p.depths[mode][level].games)) };
};
console.log(JSON.stringify({ cap: CAP, positions: all.length,
  flags: [0, 1].map((level) => describe("flags", level)), rules: [0, 1].map((level) => describe("rules", level)),
  movesNowWithRules: { median: median(all.map((p) => p.options)) },
  msPerPlay: { setup: timing.setupMs / timing.plays, run: timing.runMs / timing.plays, plays: timing.plays },
  raw: all.map((p) => ({ round: p.round, options: p.options,
    flags: p.depths.flags.map((d) => [d.distinct, d.games, d.capped]), rules: p.depths.rules.map((d) => [d.distinct, d.games, d.capped, d.stoppedAtRandom]) })),
}, null, 1));
