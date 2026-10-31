// Random-vs-random throughput on NPC decks.
// Usage: npm install && node benchmark_throughput.mjs [gameCount] [--preview] [--notify]
import { readFileSync } from "node:fs";
import { Game } from "@gi-tcg/core";
import getData from "@gi-tcg/data";
import { NPC_DECK_TABLE } from "./data_paths.mjs";

const flags = process.argv.slice(2).filter((argument) => argument.startsWith("--"));
const gameCount = Number(process.argv.slice(2).find((argument) => !argument.startsWith("--")) ?? 20);
const enablePreview = flags.includes("--preview");
const enableNotify = flags.includes("--notify");
const deckTable = JSON.parse(
  readFileSync(NPC_DECK_TABLE, "utf8"),
);
const decks = deckTable
  .filter((entry) => entry.characterList?.length === 3 && entry.cardList?.length === 30)
  .map((entry) => ({ characters: entry.characterList, cards: entry.cardList }));

let seed = process.pid;
const random = () => ((seed = (seed * 1103515245 + 12345) % 2 ** 31) / 2 ** 31);
const pick = (items) => items[Math.floor(random() * items.length)];

const data = getData();
let decisions = 0;
let rounds = 0;
let finished = 0;
const errors = new Map();

function makeIo() {
  return {
    notify: enableNotify ? () => {} : null,
    rpc: async ({ request }) => {
      decisions++;
      switch (request.$case) {
        case "chooseActive":
          return { response: { $case: "chooseActive", value: { activeCharacterId: pick(request.value.candidateIds) } } };
        case "rerollDice":
          return { response: { $case: "rerollDice", value: { diceToReroll: [] } } };
        case "switchHands":
          return { response: { $case: "switchHands", value: { removedHandIds: [] } } };
        case "selectCard":
          return { response: { $case: "selectCard", value: { selectedDefinitionId: pick(request.value.candidateDefinitionIds) } } };
        case "action": {
          const valid = request.value.action.map((action, index) => [action, index]).filter(([action]) => action.validity === 0);
          const [action, index] = pick(valid);
          return { response: { $case: "action", value: { chosenActionIndex: index, usedDice: action.autoSelectedDice } } };
        }
      }
    },
  };
}

const startedAt = performance.now();
for (let i = 0; i < gameCount; i++) {
  const state = Game.createInitialState({
    decks: [pick(decks), pick(decks)],
    data,
    randomSeed: Math.floor(random() * 2 ** 31),
  });
  const game = new Game(state, { enablePreview });
  game.players[0].io = makeIo();
  game.players[1].io = makeIo();
  try {
    await game.start();
    finished++;
    rounds += game.state.roundNumber;
  } catch (error) {
    const message = String(error?.message ?? error).slice(0, 80);
    errors.set(message, (errors.get(message) ?? 0) + 1);
  }
}
const seconds = (performance.now() - startedAt) / 1000;
console.log(`node preview=${enablePreview} notify=${enableNotify}: finished ${finished}/${gameCount} in ${seconds.toFixed(1)}s`);
console.log(`games/s ${(finished / seconds).toFixed(2)}  decisions/s ${(decisions / seconds).toFixed(0)}  avg rounds ${(rounds / finished).toFixed(2)}`);
for (const [message, count] of errors) console.log(`  error x${count}: ${message}`);
