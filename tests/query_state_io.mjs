// Restore the static handles used by queries, retaining every other internal field from the dumper.
import { StateSymbol } from "@gi-tcg/core";
export function restoreState(source, data) {
  function entity(value) {
    const definition = value.type === "character" ? data.characters.get(value.definition)
      : value.type === "attachment" ? data.attachments.get(value.definition) : data.entities.get(value.definition);
    if (!definition) throw Error(`Missing ${value.type} definition ${value.definition}`);
    return { ...value, [StateSymbol]: value.type === "character" ? "character" : value.type === "attachment" ? "attachment" : "entity",
      definition, ...("entities" in value ? { entities: value.entities.map(entity) } : {}),
      ...("attachments" in value ? { attachments: value.attachments.map(entity) } : {}) };
  }
  return { ...source, [StateSymbol]: "game", data,
    players: source.players.map((player) => ({ ...player, [StateSymbol]: "player",
      initialPile: player.initialPile.map((id) => data.entities.get(id)),
      ...Object.fromEntries(["characters", "hands", "pile", "combatStatuses", "summons", "supports", "removedEntities"]
        .map((key) => [key, player[key].map(entity)])), roundSkillLog: new Map(player.roundSkillLog),
    })), extensions: source.extensions.map((extension) => ({ ...extension, [StateSymbol]: "extension", definition: data.extensions.get(extension.definition) })) };
}
