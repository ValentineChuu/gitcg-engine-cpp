// Follow-ups: for a checked move, every distinct line of the rest of my turn after it (turn_search.mjs),
// the opponent not acting. Summarized for the network: per opponent character the most HP any line takes and whether
// any line defeats it, the most total damage any line deals, and how many lines there were, how many stopped at a draw
// or random card, and whether the budget ran out.

import { engineWork } from "./engine_work.mjs";
import { healthSnapshot, searchTurn } from "./turn_search.mjs";

export const FOLLOW_UP_BUDGET = 150;  // engine plays per checked move (a full turn has a median of 72 lines, measured with reference/tools/count_own_turn_lines.mjs)

export async function followUps(start, who, actions, index, budget = FOLLOW_UP_BUDGET) {
  const base = healthSnapshot(start);
  const found = await searchTurn(start, who, { firstMoves: [index], budget, base, actions });
  const { capped, stoppedAtRandom } = found;
  engineWork.followUps.searches += 1;
  engineWork.followUps.plays += found.plays;
  engineWork.followUps.overBudget += capped ? 1 : 0;
  const lines = found.lines.map((line) => line.change);
  const opponent = 1 - who;
  const characters = base[opponent].length;
  const mostLost = Array.from({ length: characters }, (_, position) => Math.max(0, ...lines.map((line) => -line[opponent][position][0])));
  const defeated = Array.from({ length: characters }, (_, position) => lines.some((line) => line[opponent][position][1]));
  const mostDamage = Math.max(0, ...lines.map((line) => line[opponent].reduce((total, [change]) => total - Math.min(0, change), 0)));
  return { mostLost, defeated, mostDamage, lines: lines.length, stoppedAtRandom, capped };
}
