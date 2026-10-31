// For each action of a pending action request: what the opponent could do right after it (network input "replies").
// Each valid action is played on a copy of the game. If my turn goes on (a fast action), the copy ends my turn there
// ("stopped": the danger if I stopped after this action). At the opponent's first action decision, their whole turn
// is searched (turn_search.mjs): every distinct line of their moves until their turn ends,
// what each does to my characters, and whether it needed one of their cards. The copy hides what the deciding player
// can't know: hidden cards re-dealt (redeal.mjs) and the opponent given omni dice of their real count, so every line
// they have energy for counts ("if their dice fit": an upper bound). Their dice-giving, draw and random cards are left
// out (too uncertain with a guessed hand).
//
// view (per deciding seat, from Python's create message): what that player may know about the opponent's cards.
//   { opponentDeck: "open" }      deck list public (NPC fights): their card plays come from their real remaining cards
//   { opponentDeck: "guessed", candidates: [[lineupScore, [ids]], ...] }  blind: from a guessed hand (reply_guess.mjs)
//   { opponentDeck: "hidden" }    blind without a guess: skills and switches only
// Their real hidden cards never reach a blind player's replies: card plays only use a guess there.

import { Game } from "@gi-tcg/core";
import { redealHiddenCards } from "./redeal.mjs";
import { guessedHiddenCards } from "./reply_guess.mjs";
import { seededRandom } from "./seeded_random.mjs";
import { engineWork } from "./engine_work.mjs";
import { healthSnapshot, searchTurn } from "./turn_search.mjs";

const OMNI = 8;

// The opponent's hidden cards for a copy made for `who`: a guess from tournament decks when blind with a guess, else
// null (redeal.mjs then pools their real remaining cards: fair when their list is open).
export function opponentCardsFor(state, who, seed, view, opponentPlayed) {
  if (view.opponentDeck !== "guessed") return null;
  const opponent = state.players[1 - who];
  return guessedHiddenCards(view.candidates, opponentPlayed, opponent.hands.length + opponent.pile.length, seededRandom(seed));
}

export async function opponentReplies(state, who, actions, seed, view, opponentPlayed) {
  const opponentCards = opponentCardsFor(state, who, seed, view, opponentPlayed);
  const cardSource = view.opponentDeck === "open" ? "open" : opponentCards ? "guessed" : null;
  const redealt = redealHiddenCards(state, who, seed, opponentCards);
  const players = redealt.players.map((player) => player.who === who ? player : { ...player, dice: player.dice.map(() => OMNI) });
  const start = { ...redealt, players };
  const byOutcome = new Map();  // actions that lead to the same position share one copy
  return Promise.all(actions.map((action, index) => {
    if ((action.validity ?? 0) !== 0) return null;
    const key = outcomeKey(action, index);
    if (!byOutcome.has(key)) byOutcome.set(key, replyAfter(start, who, index, action, cardSource));
    return byOutcome.get(key);
  }));
}

// Actions whose results differ only in my own hand (which card was tuned away, which copy of a card was played) give
// the opponent the same replies.
function outcomeKey(action, index) {
  const value = action.action?.value ?? {};
  switch (action.action?.$case) {
    case "elementalTuning": return `tune:${value.targetDice}`;
    case "playCard": return `card:${value.cardDefinitionId}:${JSON.stringify(value.targetIds ?? [])}:${action.autoSelectedDice}`;
    default: return `action:${index}`;
  }
}

// The opponent's whole turn after my move: play my move on a copy (after a fast move, end my turn there:
// "stopped"), stop at their first action decision, and search every distinct line of their turn (turn_search.mjs):
// their cards from their open list or a guess (none when blind without one), never their dice-giving, draw or random
// cards. Each line: what it does to my characters, counted from before my move, and whether it needed a card.
export const REPLY_BUDGET = 150;  // engine plays per checked move for the opponent's turn

function replyAfter(start, who, index, action, cardSource) {
  const game = new Game(start, {});
  let stepStart = null;
  game.onStepStart = (state) => { stepStart = state; };
  return new Promise((resolve) => {
    let answeredFirst = false;
    let stopped = false;
    let done = false;
    const finish = (result) => {
      if (done) return;
      done = true;
      resolve(result);
      game.terminate();
    };
    const opponent = 1 - who;
    for (const player of [0, 1]) {
      game.players[player].io = {
        notify: null,
        rpc: ({ request }) => {
          if (!answeredFirst && player === who && request.$case === "action") {
            answeredFirst = true;
            return Promise.resolve({ response: { $case: "action", value: { chosenActionIndex: index, usedDice: action.autoSelectedDice } } });
          }
          if (player === who && request.$case === "action" && !stopped) {
            const end = request.value.action.findIndex((offered) => offered.action?.$case === "declareEnd" && (offered.validity ?? 0) === 0);
            if (end !== -1) {
              stopped = true;  // a fast action: end my turn here and see what they could do then
              return Promise.resolve({ response: { $case: "action", value: { chosenActionIndex: end, usedDice: [] } } });
            }
          }
          const marker = stopped ? { stopped: true } : {};
          if (player === who) {
            finish({ next: "self", ...marker });
          } else if (request.$case !== "action") {
            finish({ next: stopped ? "self" : "other", ...marker });
          } else {
            const theirTurn = stepStart;
            const theirActions = request.value.action;
            done = true;  // stop this copy without answering; the search below resolves the reply
            game.terminate();
            searchTurn(theirTurn, opponent, { budget: REPLY_BUDGET, replies: true, cards: cardSource !== null,
                                              base: healthSnapshot(start), actions: theirActions })
              .then(({ lines, capped, plays }) => {
                engineWork.replies.searches += 1;
                engineWork.replies.plays += plays + 1;  // + the copy that played my move
                engineWork.replies.overBudget += capped ? 1 : 0;
                resolve({
                next: stopped ? "self" : "opponent", ...marker, capped,
                lines: lines.map((line) => ({ mine: line.change[who], usedCard: line.usedCard })),
                ...(cardSource ? { cards: cardSource } : {}),
                });
              });
            return new Promise(() => {});
          }
          return new Promise(() => {});
        },
      };
    }
    game.start().then(() => finish({ next: stopped ? "self" : "finished" }), () => finish({ next: stopped ? "self" : "finished" }));
  });
}
