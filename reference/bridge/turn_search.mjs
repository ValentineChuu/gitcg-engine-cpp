// Every distinct line of one player's moves until their turn ends (for follow-ups and replies). From a saved
// step-start state at that player's action decision, each move is played in its own copy (one engine play per move;
// copying is free, the game rules are the cost); fast moves branch further, and a line ends with the turn: a skill, a
// switch or ending the round (read right after it resolves), the end of the game, or a draw / random card (nothing is
// guessed past those). The other player never acts inside a turn. Lines reaching the same position are merged, so two
// orders of the same moves count once. Moves are trimmed by turn_line_rules.mjs. A budget caps engine plays.
//
// Each line's outcome is the change since `base` (a snapshot taken before the move being judged): per seat, per
// character, the HP change and whether it was defeated; and whether the line played a card.

import { createHash } from "node:crypto";
import { exposeState, Game } from "@gi-tcg/core";
import { branchingMoves, cardIsLeftOutOfReplies, endsLine } from "./turn_line_rules.mjs";

const DEFAULT_ANSWERS = {
  chooseActive: (value) => ({ activeCharacterId: value.candidateIds[0] }),
  rerollDice: () => ({ diceToReroll: [] }),
  switchHands: () => ({ removedHandIds: [] }),
  selectCard: (value) => ({ selectedDefinitionId: value.candidateDefinitionIds[0] }),
};
const never = () => new Promise(() => {});  // a fresh one per stopped copy, so the copy can be freed

export function healthSnapshot(state) {
  return state.players.map((player) => player.characters.map((character) => [character.variables.health, !character.variables.alive]));
}

function delta(base, state) {
  const now = healthSnapshot(state);
  return base.map((characters, seat) => characters.map(([health, defeated], index) => [now[seat][index][0] - health, now[seat][index][1] && !defeated]));
}

function fingerprint(state, player) {
  const strip = (value) => Array.isArray(value) ? value.map(strip)
    : value && typeof value === "object" ? Object.fromEntries(Object.entries(value).filter(([key]) => key !== "id" && key !== "cardId")
      .map(([key, inner]) => [key, strip(inner)])) : value;
  const view = strip(exposeState(player, state));
  for (const side of view.player) side.handCard?.sort((a, b) => (a.definitionId ?? 0) - (b.definitionId ?? 0));
  return createHash("sha1").update(JSON.stringify(view)).digest("base64");
}

// One move by `player` from `state` (their action decision at a step start; index undefined = only read the moves).
// Returns { actions } for a read, { next, actions } when their turn goes on (a fast move), or { final } with the state
// right after the move resolved when the turn (or the game) ended.
function playStep(state, player, index) {
  const game = new Game(state, {});
  let stepStart = null;
  game.onStepStart = (next) => { stepStart = next; };
  return new Promise((resolve) => {
    let answered = false;
    let endsTurn = false;  // a skill, switch or round end: even if the opponent has ended the round and it's my move again
    let done = false;
    const finish = (result) => {
      if (done) return;
      done = true;
      resolve(result);
      game.terminate();
    };
    for (const seat of [0, 1]) {
      game.players[seat].io = {
        notify: null,
        rpc: ({ request }) => {
          if (seat === player && request.$case === "action") {
            if (index === undefined) { finish({ actions: request.value.action }); return never(); }
            if (!answered) {
              answered = true;
              const chosen = request.value.action[index];
              endsTurn = !chosen.isFast;
              return Promise.resolve({ response: { $case: "action", value: { chosenActionIndex: index, usedDice: chosen.autoSelectedDice } } });
            }
            finish(endsTurn ? { final: game.state } : { next: stepStart, actions: request.value.action });
            return never();
          }
          if (seat === player && answered) {  // a choice the move itself asks for (a card to pick, ...)
            return Promise.resolve({ response: { $case: request.$case, value: DEFAULT_ANSWERS[request.$case](request.value) } });
          }
          finish({ final: game.state });  // the other player is asked: the move ended the turn
          return never();
        },
      };
    }
    game.start().then(() => finish({ final: game.state }), () => finish({ final: game.state }));
  });
}

// options: { firstMoves (indices allowed at the start, default all), budget (engine plays), replies (the searching
// player is the opponent: their dice-giving, draw and random cards are left out), cards (false: no card plays at all,
// for a blind opponent without a guess), base (snapshot to diff against, default the start), actions (the start's
// move list, if already known) }.
export async function searchTurn(start, player, options = {}) {
  const { firstMoves = null, budget = 200, replies = false, cards = true } = options;
  const base = options.base ?? healthSnapshot(start);
  let plays = 0;
  let actions = options.actions;
  if (!actions) {
    actions = (await playStep(start, player)).actions;
    plays += 1;
  }
  const lines = [];
  const seen = new Set();
  let stoppedAtRandom = 0;
  let capped = false;
  let frontier = [{ state: start, actions, root: true, usedCard: false }];
  while (frontier.length && !capped) {
    const next = [];
    for (const node of frontier) {
      // The moves asked about (firstMoves) are always played: the trimming rules only apply to the moves after them
      let moves = node.root && firstMoves ? firstMoves.filter((index) => (node.actions[index]?.validity ?? 1) === 0)
        : branchingMoves(node.actions, exposeState(player, node.state), true);
      if (replies) moves = moves.filter((index) => !cardIsLeftOutOfReplies(node.actions[index]));
      if (!cards) moves = moves.filter((index) => node.actions[index].action.$case !== "playCard");
      for (const index of moves) {
        if (plays >= budget) { capped = true; break; }
        const result = await playStep(node.state, player, index);
        plays += 1;
        const usedCard = node.usedCard || node.actions[index].action.$case === "playCard";
        if (result.final) { lines.push({ change: delta(base, result.final), usedCard }); continue; }
        if (endsLine(node.actions[index])) { stoppedAtRandom += 1; lines.push({ change: delta(base, result.next), usedCard }); continue; }
        const key = fingerprint(result.next, player);
        if (seen.has(key)) continue;
        seen.add(key);
        next.push({ state: result.next, actions: result.actions, usedCard });
      }
      if (capped) break;
    }
    frontier = next;
  }
  return { lines, plays, capped, stoppedAtRandom };
}
