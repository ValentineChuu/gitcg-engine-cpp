// Serializable internal states for test-only worker restarts. Search fixtures start without phase event logs;
// graph-bearing damage/reaction logs must use the full query fixture exporter instead.
export function snapshotState(state) {
  function entity(value) {
    return { id: value.id, definition: value.definition.id, type: value.definition.type,
      variables: { ...value.variables },
      ...(value.entities ? { entities: value.entities.map(entity) } : {}),
      ...(value.attachments ? { attachments: value.attachments.map(entity) } : {}) };
  }
  for (const side of state.players) if (side.phaseDamageLog.length || side.phaseReactionLog.length)
    throw new Error('snapshotState requires empty phase logs');
  return { ...state, prevPhase: state.prevPhase ?? null, winner: state.winner ?? null,
    players: state.players.map(player => ({ ...player, initialPile: player.initialPile.map(d => d.id),
      ...Object.fromEntries(['characters', 'hands', 'pile', 'combatStatuses', 'summons', 'supports', 'removedEntities']
        .map(key => [key, player[key].map(entity)])), roundSkillLog: [...player.roundSkillLog],
    })), extensions: state.extensions.map(extension => ({ definition: extension.definition.id, state: extension.state })), data: undefined };
}
