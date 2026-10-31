// Checks that computing action previews and the opponent's replies (opponent_replies.mjs) never changes the real game:
// the same seeds and the same seeded choices must produce identical decisions with both off (one game at a time)
// and with both on (all games at once in this process, as the workers run them; seat 0 sees the opponent's deck as
// open, seat 1 as guessed from a candidate list). Also counts how often the new reply fields appear.
// Usage: node tests/replies_equivalence.mjs [gameCount]

import { readFileSync } from "node:fs";
import getData from "@gi-tcg/data";
import { GameSession } from "../bridge/game_session.mjs";
import { seededRandom, seededShuffle } from "../bridge/seeded_random.mjs";
import { NPC_DECK_TABLE } from "../data_paths.mjs";

const gameCount = Number(process.argv[2] ?? 10);
const deckTable = JSON.parse(readFileSync(NPC_DECK_TABLE, "utf8"));
const decks = deckTable
  .filter((entry) => entry.characterList?.length === 3 && entry.cardList?.length === 30)
  .map((entry) => ({ characters: entry.characterList, cards: entry.cardList }));
const data = getData();
const seen = { replies: 0, stopped: 0, withCardReplies: 0, guessed: 0, capped: 0 };

function answer(request, random) {
  const pick = (items) => items[Math.floor(random() * items.length)];
  switch (request.$case) {
    case "chooseActive": return { $case: "chooseActive", value: { activeCharacterId: pick(request.value.candidateIds) } };
    case "rerollDice": return { $case: "rerollDice", value: { diceToReroll: [] } };
    case "switchHands": return { $case: "switchHands", value: { removedHandIds: [] } };
    case "selectCard": return { $case: "selectCard", value: { selectedDefinitionId: pick(request.value.candidateDefinitionIds) } };
    case "action": {
      const valid = request.value.action.map((action, index) => [action, index]).filter(([action]) => (action.validity ?? 0) === 0);
      const [action, index] = pick(valid);
      return { $case: "action", value: { chosenActionIndex: index, usedDice: action.autoSelectedDice } };
    }
  }
}

// One game's decisions as text: who decides, the request kind, the offered actions and the visible state.
async function playTrace(gameIndex, replies) {
  const deckRandom = seededRandom(1000 + gameIndex);
  const pair = [0, 1].map(() => decks[Math.floor(deckRandom() * decks.length)]);
  const randoms = [seededRandom(2000 + gameIndex), seededRandom(3000 + gameIndex)];
  const trace = [];
  let session;
  const finished = new Promise((resolve) => {
    const emit = (message) => {
      if (message.type === "decision") {
        // mainDamageTargetId is only sent with previews on: preview information, not part of the game.
        const actions = message.request.$case === "action" ? message.request.value.action.map((action) =>
          JSON.stringify(action.action, (name, value) => name === "mainDamageTargetId" ? undefined : value)) : [];
        trace.push(`${message.who}:${message.request.$case}:${actions.join("|")}:${JSON.stringify(message.state)}`);
        for (const reply of message.replies ?? []) {
          if (!reply) continue;
          seen.replies += 1;
          seen.stopped += reply.stopped ? 1 : 0;
          seen.withCardReplies += reply.lines?.some((line) => line.usedCard) ? 1 : 0;
          seen.guessed += reply.cards === "guessed" ? 1 : 0;
          seen.capped += reply.capped ? 1 : 0;
        }
        session.respond(message.who, answer(message.request, randoms[message.who]));
      } else if (message.type === "finished" || message.type === "failed") {
        trace.push(`${message.type}:${message.winner ?? message.message}`);
        resolve();
      }
    };
    const views = [{ opponentDeck: "open" }, { opponentDeck: "guessed", candidates: [[4, pair[0].cards], [0, pair[1].cards]] }];
    session = GameSession.create(`g${gameIndex}`, pair, 7919 * (gameIndex + 1), data, emit, [], null, replies, replies, views);
    session.run();
  });
  await finished;
  return trace;
}

const indices = [...Array(gameCount).keys()];
const offTraces = [];
for (const gameIndex of indices) offTraces.push(await playTrace(gameIndex, false));
const onTraces = await Promise.all(indices.map((gameIndex) => playTrace(gameIndex, true)));
let differing = 0;
for (const gameIndex of indices) {
  const [off, on] = [offTraces[gameIndex], onTraces[gameIndex]];
  const first = off.findIndex((line, index) => line !== on[index]);
  if (first === -1 && off.length === on.length) continue;
  differing += 1;
  console.log(`game ${gameIndex}: first difference at decision ${first} of ${off.length}/${on.length}`);
  console.log(`  off: ${off[first]?.slice(0, 400)}`);
  console.log(`  on:  ${on[first]?.slice(0, 400)}`);
}
console.log(`${gameCount - differing}/${gameCount} games identical with replies on and off`);
console.log(`replies: ${seen.replies} actions, ${seen.stopped} measured after ending the turn, ${seen.withCardReplies} with card replies, ${seen.guessed} with guessed cards, ${seen.capped} over budget`);
process.exit(differing ? 1 : 0);
