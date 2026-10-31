// Controlled internal states for slot coverage. These are rule probes, not legal starting decks.
import { Game, StateSymbol } from '@gi-tcg/core';

export function makeProbe(data, probe, seed) {
  const definition = data.characters.get(probe.definition) ?? data.entities.get(probe.definition) ?? data.attachments.get(probe.definition);
  const related = Math.floor(probe.definition / 10) % 10000;
  const first = definition.type === 'character' ? definition.id : data.characters.has(related) ? related : 1101;
  const characters = probe.characters ?? [first, ...[1101, 1201, 1301].filter(id => id !== first)].slice(0, 3);
  const isCard = ['eventCard', 'equipment', 'support'].includes(definition.type);
  const cards = [...(isCard && !probe.equipped ? [definition.id] : []), ...(probe.cards ?? [])];
  const decks = [{ characters, cards, noShuffle: true },
                 { characters: probe.opponents ?? [1301, 1401, 1201], cards: [], noShuffle: true }];
  let game = new Game(Game.createInitialState({ data, decks, randomSeed: seed }));
  for (const card of [...game.state.players[0].pile]) {
    game.mutate({ type: 'moveEntity', from: { type: 'pile', who: 0 }, target: { type: 'hands', who: 0 }, value: card, reason: 'draw' });
  }
  function place(id, who = 0, characterIndex = 0, variables = {}) {
    const def = data.entities.get(id);
    if (!def) throw new Error(`Unknown probe entity ${id}`);
    const characterId = game.state.players[who].characters[characterIndex].id;
    const area = ['status', 'equipment'].includes(def.type) ? { type: 'characters', who, characterId }
      : { type: def.type === 'combatStatus' ? 'combatStatuses' : def.type === 'summon' ? 'summons' : 'supports', who };
    const entity = { [StateSymbol]: 'entity', id: 0, definition: def,
      variables: { ...Object.fromEntries(Object.entries(def.varConfigs).map(([name, config]) => [name, config.initialValue])), ...variables }, attachments: [] };
    game.mutate({ type: 'createEntity', target: area, value: entity });
  }
  if ((!isCard && definition.type !== 'character' && definition.type !== 'attachment') || probe.equipped)
    place(definition.id, 0, 0, probe.variables);
  if (probe.talent !== false) {
    for (const [index, id] of characters.entries()) {
      const talent = [...data.entities.values()].find(entity => entity.tags.includes('talent') && Math.floor(entity.id / 10) % 10000 === id);
      if (talent && talent.id !== definition.id) place(talent.id, 0, index);
    }
  }
  for (const entity of probe.entities ?? []) place(entity.definition, entity.who ?? 0, entity.characterIndex ?? 0, entity.variables);
  const players = game.state.players.map((player, who) => ({ ...player,
    activeCharacterId: player.characters[probe.activeIndices?.[who] ?? 0].id, dice: Array(probe.dice ?? 16).fill(8),
    characters: player.characters.map((character, index) => ({ ...character,
      variables: { ...character.variables, energy: character.variables.maxEnergy,
        health: probe.health?.[who]?.[index] ?? (who === 0 ? 4 : 10),
        ...(probe.auras?.[who]?.[index] !== undefined ? { aura: probe.auras[who][index] } : {}),
        ...(probe.characterVariables?.[who]?.[index] ?? {}) },
    })),
  }));
  game = new Game({ ...game.state, phase: 'action', prevPhase: probe.phaseStart === false ? 'action' : 'roll',
    roundNumber: 1, currentTurn: probe.turn ?? 0, players });
  return { game, decks };
}
