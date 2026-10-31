// Counts of engine work by purpose, for profiling (benchmarks/reply_cost.py): copies started and engine plays (one
// move played in a copy) for replies, follow-ups and the end of round. Printed to stderr at shutdown when the
// environment has AUTOGITCG_ENGINE_WORK=1.

export const engineWork = {
  decisions: 0,
  replies: { searches: 0, plays: 0, overBudget: 0 },
  followUps: { searches: 0, plays: 0, overBudget: 0 },
  endOfRound: { copies: 0 },
};

export function reportEngineWork() {
  if (process.env.AUTOGITCG_ENGINE_WORK === "1") process.stderr.write(`ENGINE_WORK ${JSON.stringify(engineWork)}\n`);
}
