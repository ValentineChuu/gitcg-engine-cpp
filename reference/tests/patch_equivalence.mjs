// Checks that the core patches only change speed, not game behavior:
// the same seeds must produce identical decision traces and winners with previews/notifications on or off.
// Usage: node tests/patch_equivalence.mjs [gameCount]

import { readFileSync } from "node:fs";
import { Game } from "@gi-tcg/core";
import getData from "@gi-tcg/data";
import { seededRandom, seededShuffle } from "../bridge/seeded_random.mjs";
import { NPC_DECK_TABLE } from "../data_paths.mjs";

const gameCount = Number(process.argv[2] ?? 30);
const deckTable = JSON.parse(readFileSync(NPC_DECK_TABLE, "utf8"));
const decks = deckTable
  .filter((entry) => entry.characterList?.length === 3 && entry.cardList?.length === 30)
  .map((entry) => ({ characters: entry.characterList, cards: entry.cardList }));
const data = getData();

function makeRecordingIo(random, trace, who, enableNotify) {
  const pick = (items) => items[Math.floor(random() * items.length)];
  return {
    notify: enableNotify ? () => {} : null,
    rpc: async ({ request }) => {
      switch (request.$case) {
        case "chooseActive": {
          const id = pick(request.value.candidateIds);
          trace.push(`${who}:active:${id}`);
          return { response: { $case: "chooseActive", value: { activeCharacterId: id } } };
        }
        case "rerollDice":
          return { response: { $case: "rerollDice", value: { diceToReroll: [] } } };
        case "switchHands":
          return { response: { $case: "switchHands", value: { removedHandIds: [] } } };
        case "selectCard": {
          const id = pick(request.value.candidateDefinitionIds);
          trace.push(`${who}:select:${id}`);
          return { response: { $case: "selectCard", value: { selectedDefinitionId: id } } };
        }
        case "action": {
          const actions = request.value.action;
          const valid = actions.map((action, index) => [action, index]).filter(([action]) => action.validity === 0);
          const [action, index] = pick(valid);
          trace.push(`${who}:action:${index}/${actions.length}:${action.autoSelectedDice.join(",")}`);
          return { response: { $case: "action", value: { chosenActionIndex: index, usedDice: action.autoSelectedDice } } };
        }
      }
    },
  };
}

async function playTrace(gameIndex, enablePreview, enableNotify) {
  const deckRandom = seededRandom(1000 + gameIndex);
  // The core's own initial shuffle uses Math.random, so shuffle with a seeded RNG and pass noShuffle.
  const deckPair = [0, 1].map(() => {
    const deck = decks[Math.floor(deckRandom() * decks.length)];
    return { characters: deck.characters, cards: seededShuffle(deck.cards, deckRandom), noShuffle: true };
  });
  const state = Game.createInitialState({ decks: deckPair, data, randomSeed: 7919 * (gameIndex + 1) });
  const game = new Game(state, { enablePreview });
  const trace = [];
  game.players[0].io = makeRecordingIo(seededRandom(2000 + gameIndex), trace, 0, enableNotify);
  game.players[1].io = makeRecordingIo(seededRandom(3000 + gameIndex), trace, 1, enableNotify);
  const winner = await game.start();
  return { winner, trace };
}

const modes = [
  { enablePreview: true, enableNotify: true },
  { enablePreview: true, enableNotify: true },
  { enablePreview: false, enableNotify: false },
  { enablePreview: false, enableNotify: true },
  { enablePreview: true, enableNotify: false },
];
let mismatches = 0;
for (let gameIndex = 0; gameIndex < gameCount; gameIndex++) {
  const results = [];
  for (const mode of modes) results.push(await playTrace(gameIndex, mode.enablePreview, mode.enableNotify));
  const [reference, ...others] = results;
  others.forEach((other, offset) => {
    const mode = modes[offset + 1];
    const sameTrace = other.trace.join("|") === reference.trace.join("|");
    if (!sameTrace || other.winner !== reference.winner) {
      mismatches++;
      const divergeAt = other.trace.findIndex((entry, index) => entry !== reference.trace[index]);
      console.log(`game ${gameIndex} mode ${JSON.stringify(mode)}: diverged at decision ${divergeAt}`);
      console.log(`  reference: ${reference.trace[divergeAt]}  got: ${other.trace[divergeAt]}`);
    }
  });
}
console.log(mismatches === 0 ? `OK: ${gameCount} games identical across all modes` : `FAIL: ${mismatches} mismatches`);
process.exitCode = mismatches === 0 ? 0 : 1;
