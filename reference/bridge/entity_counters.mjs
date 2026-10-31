// The exposed state shows only one counter per board entity, but the game client shows all of them
// (e.g. usages and remaining duration). Collect those counters from the internal state by entity id.

// Counters that the client displays; internal bookkeeping variables are left out.
// Keep in sync with DESCRIPTIVE_VARIABLES in autogitcg/observation/catalog_features.py.
export const DISPLAYED_COUNTERS = ["usage", "usagePerRound", "duration", "shield", "progress", "layer", "nightsoul", "count", "barrierUsage"];

export function boardCountersById(gameState) {
  const countersById = {};
  const collect = (entity) => {
    const counters = {};
    for (const name of DISPLAYED_COUNTERS) {
      if (typeof entity.variables?.[name] === "number") counters[name] = entity.variables[name];
    }
    countersById[entity.id] = counters;
  };
  for (const player of gameState.players) {
    for (const character of player.characters) character.entities.forEach(collect);
    player.combatStatuses.forEach(collect);
    player.summons.forEach(collect);
    player.supports.forEach(collect);
  }
  return countersById;
}
