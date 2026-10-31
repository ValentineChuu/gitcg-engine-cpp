// Seeded decision snapshots of game.state, with definition references and insertion-ordered variables.
// Usage: node tests/dump_internal_states.mjs --games 40 --seed 1
import { mkdirSync, readFileSync, renameSync, writeFileSync } from "node:fs";
import { parseArgs } from "node:util";
import getData from "@gi-tcg/data";
import { GameSession } from "../reference/bridge/game_session.mjs";
import { seededRandom } from "../reference/bridge/seeded_random.mjs";
import { pairDecks } from "./deck_pairing.mjs";
import { randomResponse } from "./random_responder.mjs";

const { values: options } = parseArgs({ options: {
  games: { type: "string", default: "40" }, seed: { type: "string", default: "1" },
  sources: { type: "string", default: "tournament,npc,stage,synergy" },
  decks: { type: "string", default: new URL("generated/test_decks.json", import.meta.url).pathname },
  output: { type: "string", default: new URL("query_generated/states.json", import.meta.url).pathname },
  every: { type: "string", default: "10" }, "max-states": { type: "string", default: "1200" },
} });
const data = getData();
const decks = JSON.parse(readFileSync(options.decks, "utf8"));
const sources = options.sources.split(",");
const seed = Number(options.seed), every = Number(options.every), maxStates = Number(options["max-states"]);
if (!Number.isInteger(every) || every < 1 || !Number.isInteger(maxStates) || maxStates < 1) throw Error("Invalid sampling limits");

export function dumpEntity(entity) {
  return { id: entity.id, definition: entity.definition.id, type: entity.definition.type,
    variables: { ...entity.variables },
    ...("entities" in entity ? { entities: entity.entities.map(dumpEntity) } : {}),
    ...("attachments" in entity ? { attachments: entity.attachments.map(dumpEntity) } : {}) };
}
export function dumpState(state) {
  // Event arguments can point at historical states or each other. Keep their complete JSON graph as references.
  const seen = new WeakMap([[state, "#"]]);
  function graph(value, path) {
    if (value === null || typeof value !== "object") return typeof value === "function" ? { $function: true } : value;
    if (seen.has(value)) return { $ref: seen.get(value) };
    if (value === data) return { $gameData: true };
    if ("skills" in value && "id" in value) return { $definition: value.id, type: value.type ?? "extension" };
    seen.set(value, path);
    if (value instanceof Map) return [...value].map(([k, v], i) => [k, graph(v, `${path}/${i}/1`)]);
    if (value instanceof Set) return [...value].map((v, i) => graph(v, `${path}/${i}`));
    if (Array.isArray(value)) return value.map((v, i) => graph(v, `${path}/${i}`));
    return Object.fromEntries(Object.entries(value).map(([k, v]) => [k, graph(v, `${path}/${k}`)]));
  }
  const { data: ignored, ...root } = state;
  return { ...root, players: state.players.map((player, who) => ({ ...player,
    initialPile: player.initialPile.map((def) => def.id),
    characters: player.characters.map(dumpEntity),
    ...Object.fromEntries(["hands", "pile", "combatStatuses", "summons", "supports", "removedEntities"]
      .map((key) => [key, player[key].map(dumpEntity)])),
    roundSkillLog: [...player.roundSkillLog].map(([id, skills]) => [id, [...skills]]),
    phaseDamageLog: graph(player.phaseDamageLog, `#/players/${who}/phaseDamageLog`),
    phaseReactionLog: graph(player.phaseReactionLog, `#/players/${who}/phaseReactionLog`),
  })), extensions: state.extensions.map((extension) => ({ definition: extension.definition.id, state: extension.state })) };
}

const snapshots = [], failures = [], games = [];
let bytes = 0, decisions = 0;
for (let index = 0; index < Number(options.games); ++index) {
  const random = seededRandom(seed * 1_000_003 + index);
  // Cycle sources so even small runs cover all four origins.
  const pairing = pairDecks(decks, [sources[index % sources.length]], random);
  let decision = 0;
  await new Promise((resolve) => {
    let session;
    const emit = (message) => {
      if (message.type === "decision") {
        if ((decision % every === 0 || message.request.$case === "chooseActive") && snapshots.length < maxStates) {
          const snapshot = { game: index, decision, origins: pairing.origins, state: dumpState(session.game.state) };
          const size = Buffer.byteLength(JSON.stringify(snapshot));
          if (bytes + size < 48_000_000) { snapshots.push(snapshot); bytes += size; }
        }
        ++decision; ++decisions;
        const response = randomResponse(message, random);
        queueMicrotask(() => session.respond(message.who, response));
      } else if (message.type === "finished" || message.type === "failed") {
        games.push({ index, origins: pairing.origins, outcome: message.type, decisions: decision });
        if (message.type === "failed") failures.push({ index, message: message.message });
        resolve();
      }
    };
    try {
      session = GameSession.create(index, pairing.decks, Math.floor(random() * 2 ** 31), data, emit, pairing.equipment);
      session.run();
    } catch (error) { failures.push({ index, message: String(error.stack ?? error) }); resolve(); }
  });
}
mkdirSync(new URL("query_generated/", import.meta.url), { recursive: true });
writeFileSync(options.output + ".tmp", JSON.stringify({ seed, games, decisions, failures, snapshots }) + "\n");
renameSync(options.output + ".tmp", options.output);
console.log(`${games.length} games, ${failures.length} failed, ${decisions} decisions, ${snapshots.length} states, ${bytes} bytes -> ${options.output}`);
if (failures.length) process.exitCode = 1;
