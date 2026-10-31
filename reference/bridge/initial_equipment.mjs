// Equip cards on characters before the game starts (e.g. a Serious Showdown NPC starts with its talent).
// The entity is placed directly, as if already equipped: its "when played" effects do not trigger.

import { StateSymbol } from "@gi-tcg/core";

function initialVariables(definition) {
  return Object.fromEntries(Object.entries(definition.varConfigs ?? {}).map(([name, config]) => [name, config.initialValue]));
}

// equipment: [{ who, characterIndex, definitionId }]
export function applyInitialEquipment(game, data, equipment) {
  for (const { who, characterIndex, definitionId } of equipment) {
    const definition = data.entities.get(definitionId);
    if (!definition) throw new Error(`unknown equipment definition ${definitionId}`);
    const character = game.state.players[who].characters[characterIndex];
    if (!character) throw new Error(`player ${who} has no character at index ${characterIndex}`);
    game.mutate({
      type: "createEntity",
      target: { type: "characters", who, characterId: character.id },
      value: { [StateSymbol]: "entity", id: 0, definition, variables: initialVariables(definition), attachments: [] },
    });
  }
}
