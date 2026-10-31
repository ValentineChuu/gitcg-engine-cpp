// Play random games with the given characters as player 1's lineup and summarize what each of their
// skills did, to check custom definitions against their descriptions.
// Usage: node tools/try_characters.mjs [--games N] <characterId> [<characterId> ...]
//   Each id gets its own games; join ids with "+" to play them together, e.g. 4301+4302.

import { readFileSync } from "node:fs";
import { exposeState, Game } from "@gi-tcg/core";
import getData from "@gi-tcg/data";
import { seededRandom, seededShuffle } from "../bridge/seeded_random.mjs";
import { NPC_DECK_TABLE } from "../data_paths.mjs";

const argumentsList = process.argv.slice(2);
const gamesFlag = argumentsList.indexOf("--games");
const gamesPerLineup = gamesFlag >= 0 ? Number(argumentsList.splice(gamesFlag, 2)[1]) : 20;
const lineups = argumentsList.flatMap((argument) => argument.split(/\s+/)).filter(Boolean)
  .map((argument) => argument.split("+").map(Number));
const data = getData();
const playerDecks = JSON.parse(readFileSync(NPC_DECK_TABLE, "utf8"))
  .filter((entry) => entry.characterList?.length === 3 && entry.cardList?.length === 30)
  .map((entry) => ({ characters: entry.characterList, cards: entry.cardList }));

function snapshot(state) {
  const exposed = exposeState(1, state);
  return exposed.player.map((player) => ({
    characters: player.character.map((character) => ({ health: character.health, energy: character.energy, aura: character.aura,
      entities: character.entity.map((entity) => entity.definitionId) })),
    combat: player.combatStatus.map((entity) => entity.definitionId),
    summons: player.summon.map((entity) => entity.definitionId),
  }));
}

function describeChange(before, after) {
  const parts = [];
  for (const side of [0, 1]) {
    const label = side === 1 ? "self" : "opponent";
    after[side].characters.forEach((character, index) => {
      const previous = before[side].characters[index];
      if (!previous) return;
      if (character.health !== previous.health) parts.push(`${label}[${index}] hp ${character.health - previous.health}`);
      if (character.energy !== previous.energy) parts.push(`${label}[${index}] energy ${character.energy - previous.energy}`);
      const added = character.entities.filter((id) => !previous.entities.includes(id));
      if (added.length) parts.push(`${label}[${index}] +status ${added.join(",")}`);
    });
    for (const zone of ["combat", "summons"]) {
      const added = after[side][zone].filter((id) => !before[side][zone].includes(id));
      if (added.length) parts.push(`${label} +${zone} ${added.join(",")}`);
    }
  }
  return parts.sort().join("; ") || "(no visible change)";
}

for (const lineup of lineups) {
  const outcomes = new Map(); // skill id -> Map(description -> count)
  const failures = new Map();
  for (let gameIndex = 0; gameIndex < gamesPerLineup; gameIndex++) {
    const random = seededRandom(1000 + gameIndex);
    const pick = (items) => items[Math.floor(random() * items.length)];
    const opponentDeck = pick(playerDecks);
    const decks = [
      { characters: opponentDeck.characters, cards: seededShuffle(opponentDeck.cards, random), noShuffle: true },
      { characters: lineup, cards: [], noShuffle: true },
    ];
    let pendingSkill = null;
    let game;
    try {
      game = new Game(Game.createInitialState({ decks, data, randomSeed: 7 * (gameIndex + 1) }));
    } catch (error) {
      const message = `create: ${String(error?.message ?? error).split("\n")[0]}`;
      failures.set(message, (failures.get(message) ?? 0) + 1);
      continue;
    }
    const finishPending = () => {
      if (!pendingSkill) return;
      const description = describeChange(pendingSkill.before, snapshot(game.state));
      const counts = outcomes.get(pendingSkill.id) ?? new Map();
      counts.set(description, (counts.get(description) ?? 0) + 1);
      outcomes.set(pendingSkill.id, counts);
      pendingSkill = null;
    };
    for (const who of [0, 1]) {
      game.players[who].io = {
        notify: null,
        rpc: async ({ request }) => {
          finishPending(); // the next decision by either player comes right after the skill fully resolved
          const value = request.value;
          switch (request.$case) {
            case "chooseActive": return { response: { $case: "chooseActive", value: { activeCharacterId: pick(value.candidateIds) } } };
            case "rerollDice": return { response: { $case: "rerollDice", value: { diceToReroll: [] } } };
            case "switchHands": return { response: { $case: "switchHands", value: { removedHandIds: [] } } };
            case "selectCard": return { response: { $case: "selectCard", value: { selectedDefinitionId: pick(value.candidateDefinitionIds) } } };
            case "action": {
              const valid = value.action.map((action, index) => [action, index]).filter(([action]) => action.validity === 0);
              const skills = valid.filter(([action]) => action.action.$case === "useSkill");
              // Player 1 prefers using skills so each one gets exercised.
              const [action, index] = who === 1 && skills.length && random() < 0.8 ? pick(skills) : pick(valid);
              if (who === 1 && action.action.$case === "useSkill") {
                pendingSkill = { id: action.action.value.skillDefinitionId, before: snapshot(game.state) };
              }
              return { response: { $case: "action", value: { chosenActionIndex: index, usedDice: action.autoSelectedDice } } };
            }
          }
        },
      };
    }
    try {
      await game.start();
      finishPending();
    } catch (error) {
      const message = String(error?.message ?? error).split("\n")[0];
      failures.set(message, (failures.get(message) ?? 0) + 1);
    }
  }
  console.log(`\n== lineup ${lineup.join("+")}: ${gamesPerLineup} games, ${[...failures.values()].reduce((a, b) => a + b, 0)} failed`);
  for (const [message, count] of failures) console.log(`  FAILURE x${count}: ${message}`);
  for (const [skillId, counts] of [...outcomes].sort((a, b) => a[0] - b[0])) {
    console.log(`  skill ${skillId}:`);
    for (const [description, count] of [...counts].sort((a, b) => b[1] - a[1]).slice(0, 4)) console.log(`    x${count} ${description}`);
  }
}
