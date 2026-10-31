// Records reference games from the patched Node engine for the C++ equivalence tests.
// Each game is driven through the real bridge (GameSession) with seeded random answers, and every message is kept
// in order: the worker's "create" input, then {"out": message} for what the worker sends and {"in": message} for the
// answers. Replaying the "in" messages into the C++ worker must reproduce every "out" message exactly.
//
// Usage: node tests/record_reference.mjs --games 200 --seed 1 [--sources tournament,npc,stage,synergy]
//        [--previews] [--output tests/generated/reference_<seed>.jsonl.gz]
// Decks come from export_test_decks.py.

import { createWriteStream, readFileSync } from "node:fs";
import { parseArgs } from "node:util";
import { createGzip } from "node:zlib";
import getData from "@gi-tcg/data";
import { GameSession } from "../reference/bridge/game_session.mjs";
import { seededRandom } from "../reference/bridge/seeded_random.mjs";
import { pairDecks } from "./deck_pairing.mjs";
import { randomResponse } from "./random_responder.mjs";

const { values: options } = parseArgs({
  options: {
    games: { type: "string", default: "100" },
    seed: { type: "string", default: "1" },
    sources: { type: "string", default: "tournament,npc,stage,synergy" },
    previews: { type: "boolean", default: false },
    decks: { type: "string", default: new URL("generated/test_decks.json", import.meta.url).pathname },
    output: { type: "string" },
  },
});
const seed = Number(options.seed);
const output = options.output ?? new URL(`generated/reference_${seed}.jsonl.gz`, import.meta.url).pathname;
const decks = JSON.parse(readFileSync(options.decks, "utf8"));
const sources = options.sources.split(",");
const data = getData();

// Plays one game; resolves with its messages in order.
function recordGame(gameIndex) {
  const random = seededRandom(seed * 1_000_003 + gameIndex);
  const { decks: deckPair, equipment, origins } = pairDecks(decks, sources, random);
  const create = { type: "create", game: gameIndex, decks: deckPair, seed: Math.floor(random() * 2 ** 31), equipment,
                   ...(options.previews ? { previews: true } : {}) };
  const messages = [{ create, origins }];
  return new Promise((resolve) => {
    let session;
    const emit = (message) => {
      messages.push({ out: message });
      if (message.type === "decision") {
        const response = randomResponse(message, random);
        const answer = { type: "respond", game: gameIndex, who: message.who, response };
        messages.push({ in: answer });
        queueMicrotask(() => session.respond(message.who, response));
      } else if (message.type === "finished" || message.type === "failed") {
        resolve(messages);
      }
    };
    try {
      session = GameSession.create(gameIndex, deckPair, create.seed, data, emit, equipment, null, options.previews);
    } catch (error) {
      messages.push({ out: { type: "failed", game: gameIndex, message: String(error?.stack ?? error) } });
      resolve(messages);
      return;
    }
    session.run();
  });
}

const gzip = createGzip();
gzip.pipe(createWriteStream(output));
const gameCount = Number(options.games);
let failures = 0;
const started = Date.now();
for (let gameIndex = 0; gameIndex < gameCount; gameIndex++) {
  const messages = await recordGame(gameIndex);
  if (messages.at(-1).out?.type === "failed") failures++;
  for (const message of messages) gzip.write(JSON.stringify(message) + "\n");
}
gzip.end();
console.log(`${gameCount} games (${failures} failed) in ${((Date.now() - started) / 1000).toFixed(1)} s -> ${output}`);
