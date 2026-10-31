// Rules that cut moves from own-turn lines before the engine plays them: useless cards and random
// effects. Card facts (food, healing, draws, random effects, dice-giving) come from the engine's data/card_line_facts.json,
// exported from the GTS card code by tools/export_card_facts.mjs.

import { existsSync, readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";

const SATIATED = 303300;
// Exported from the GTS card code by tools/export_card_facts.mjs (reference/piovium isn't copied to cloud machines).
const FACTS_FILE = fileURLToPath(new URL("../../data/card_line_facts.json", import.meta.url));

function readCardFacts() {
  if (!existsSync(FACTS_FILE)) {
    throw new Error(`${FACTS_FILE} is missing: run node tools/export_card_facts.mjs (the line rules would trim nothing)`);
  }
  return new Map(Object.entries(JSON.parse(readFileSync(FACTS_FILE, "utf8"))).map(([id, facts]) => [Number(id), facts]));
}

export const cardFacts = readCardFacts();

// After playing this card the line can't be followed without guessing (drawn cards, random results), so it stops.
// For the opponent's replies these cards are left out entirely (cardIsLeftOutOfReplies), along with dice-giving ones:
// their hand is a guess, so what they would draw or which dice they'd get is too uncertain to count on.
export function cardIsLeftOutOfReplies(action) {
  if (action.action.$case !== "playCard") return false;
  const facts = cardFacts.get(action.action.value.cardDefinitionId);
  return Boolean(facts && (facts.draws || facts.random || facts.givesDice));
}

export function endsLine(action) {
  if (action.action.$case !== "playCard") return false;
  const facts = cardFacts.get(action.action.value.cardDefinitionId);
  return Boolean(facts && (facts.draws || facts.random));
}

// The moves a line branches on. Always: valid and not marked "does nothing" by the engine; tunings cut to one per
// target die (tuning away a card that can't be played now, if there is one). With rules: healing food on a full-HP
// target and any food on a Satiated target are cut, and copies of the same card with the same targets and payment
// count once.
export function branchingMoves(actions, view, useRules) {
  const indices = actions.map((action, index) => [action, index])
    .filter(([action]) => (action.validity ?? 0) === 0 && !action.action.value?.willBeEffectless);
  const playable = new Set(indices.filter(([action]) => action.action.$case === "playCard").map(([action]) => action.action.value.cardId));
  const tuneByDie = new Map();
  for (const [action, index] of indices) {
    if (action.action.$case !== "elementalTuning") continue;
    const die = action.action.value.targetDice;
    const current = tuneByDie.get(die);
    if (current === undefined || (playable.has(actions[current].action.value.removedCardId) && !playable.has(action.action.value.removedCardId))) {
      tuneByDie.set(die, index);
    }
  }
  const keptTunings = new Set(tuneByDie.values());
  const characters = new Map(view.player.flatMap((player) => player.character).map((character) => [character.id, character]));
  const seenCards = new Set();
  return indices.filter(([action, index]) => {
    const value = action.action.value;
    if (action.action.$case === "elementalTuning") return keptTunings.has(index);
    if (!useRules || action.action.$case !== "playCard") return true;
    const facts = cardFacts.get(value.cardDefinitionId);
    if (facts?.food) {
      const target = characters.get((value.targetIds ?? [])[0]);
      if (target && (target.entity ?? []).some((entity) => entity.definitionId === SATIATED)) return false;
      if (target && facts.heals && target.health >= target.maxHealth) return false;
    }
    const key = `${value.cardDefinitionId}:${JSON.stringify(value.targetIds ?? [])}:${action.autoSelectedDice}`;
    if (seenCards.has(key)) return false;
    seenCards.add(key);
    return true;
  }).map(([, index]) => index);
}
