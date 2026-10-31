// Controlled games for every resolved definition: characters in the lineup, playable cards in the opening deck,
// and other entities placed in their proper board zone. These are synthetic rule probes, not legal deck lists.
// The report distinguishes presence, offered actions, chosen actions and triggered skills; mere definition loading
// is not claimed as behavioral coverage. Replay with --snapshots and then run sweep_coverage.mjs.
import { createWriteStream, writeFileSync } from 'node:fs';
import { once } from 'node:events';
import { createGzip } from 'node:zlib';
import { parseArgs } from 'node:util';
import { Game, StateSymbol } from '@gi-tcg/core';
import getData from '@gi-tcg/data';
import { GameSession } from '../reference/bridge/game_session.mjs';
import { seededRandom } from '../reference/bridge/seeded_random.mjs';
import { snapshotState } from './state_snapshot.mjs';

const { values: options } = parseArgs({ options: {
  output: { type: 'string', default: 'tests/generated/sweep_reference.jsonl.gz' },
  coverage: { type: 'string', default: 'tests/generated/sweep_node_coverage.json' },
  limit: { type: 'string' }, seed: { type: 'string', default: '601' },
  ids: { type: 'string' }, 'opening-hands': { type: 'boolean' },
  rounds: { type: 'string', default: '3' }, decisions: { type: 'string', default: '50' },
} });
for (const name of ['rounds', 'decisions']) if (!Number.isInteger(Number(options[name])) || Number(options[name]) < 1)
  throw new Error(`--${name} must be a positive integer`);
const data = getData();
const definitions = [...data.characters.values(), ...data.entities.values(), ...data.attachments.values()];
const output = createWriteStream(options.output), gzip = createGzip(); gzip.pipe(output);
const coverage = [];
let gameIndex = 0;
const started = performance.now();
const selected = options.ids ? definitions.filter(d => options.ids.split(',').map(Number).includes(d.id)) : definitions;
for (const definition of selected.slice(0, Number(options.limit ?? selected.length))) {
  const id = gameIndex++, random = seededRandom(Number(options.seed) + id);
  const record = { id: definition.id, type: definition.type, game: id, present: [], offered: [], chosen: [], triggered: [], seenAt: [], triggeredAt: [], chosenAt: [], failure: null };
  const related = Math.floor(definition.id / 10) % 10000;
  const first = definition.type === 'character' ? definition.id : data.characters.has(related) ? related : 1101;
  const characters = [first, ...[1101, 1201, 1301].filter(id => id !== first)].slice(0, 3);
  const card = definition.type === 'eventCard' || definition.type === 'equipment' || definition.type === 'support';
  const decks = [{ characters, cards: card ? [definition.id] : definition.type === 'attachment' ? [332004] : [], noShuffle: true },
                 { characters: [1101, 1201, 1301], cards: [], noShuffle: true }];
  const game = new Game(Game.createInitialState({ data, decks, randomSeed: Number(options.seed) + id }));
  if (card && options['opening-hands']) {
    const target = game.state.players[0].pile[0];
    game.mutate({ type: 'moveEntity', from: { type: 'pile', who: 0 }, target: { type: 'hands', who: 0 }, value: target, reason: 'draw' });
  }
  // Begin with enough health and energy to let healing and burst branches appear in short runs.
  for (const side of game.state.players) for (const character of side.characters) {
    game.mutate({ type: 'modifyEntityVar', state: character, varName: 'health', value: 8, oldValue: 0, direction: null });
    game.mutate({ type: 'modifyEntityVar', state: character, varName: 'energy', value: character.variables.maxEnergy, oldValue: 0, direction: null });
  }
  if (!card && definition.type !== 'character') {
    const area = definition.type === 'status' ? { type: 'characters', who: 0, characterId: game.state.players[0].characters[0].id }
      : { type: definition.type === 'combatStatus' ? 'combatStatuses' : definition.type === 'summon' ? 'summons' : 'supports', who: 0 };
    const entity = { [StateSymbol]: definition.type === 'attachment' ? 'attachment' : 'entity', id: 0, definition,
      variables: Object.fromEntries(Object.entries(definition.varConfigs).map(([name, config]) => [name, config.initialValue])), attachments: [] };
    if (definition.type === 'attachment') game.mutate({ type: 'createAttachment', target: { type: 'pile', who: 0, cardId: game.state.players[0].pile[0].id }, value: entity });
    else game.mutate({ type: 'createEntity', target: area, value: entity });
  }
  const create = { type: 'create', game: id, decks, seed: Number(options.seed) + id, initialState: snapshotState(game.state) };
  gzip.write(JSON.stringify({ create, origins: [`sweep:${definition.type}:${definition.id}`] }) + '\n');
  // Core notifications expose skillUsed, allowing a separate list of definitions that were present but never acted.
  let decisionCount = 0;
  const collect = notification => {
    for (const entry of notification.mutation ?? []) {
      const mutation = entry.mutation;
      if (mutation?.$case === 'skillUsed') {
        record.triggered.push(mutation.value.callerDefinitionId);
        record.triggeredAt.push({ id: mutation.value.callerDefinitionId, afterDecision: decisionCount });
      }
    }
  };
  const messages = [];
  await new Promise(resolve => {
    let session, decisions = 0;
    const emit = message => {
      messages.push({ out: message });
      if (message.type !== 'decision') { if (message.type === 'failed') record.failure = message.message; resolve(); return; }
      ++decisions; decisionCount = decisions;
      const seen = new Set();
      const visit = value => {
        if (!value || typeof value !== 'object') return;
        if (typeof value.definitionId === 'number' && value.definitionId !== 0) seen.add(value.definitionId);
        for (const child of Object.values(value)) if (typeof child === 'object') visit(child);
      };
      visit(message.state);
      for (const value of seen) if (!record.present.includes(value)) { record.present.push(value); record.seenAt.push({ id: value, decision: decisions }); }
      if (message.request.$case === 'action' && (decisions >= Number(options.decisions) || message.state.roundNumber >= Number(options.rounds))) {
        messages.push({ in: { type: 'stop', game: id } }); session.stop(); resolve(); return;
      }
      const request = message.request;
      let value;
      if (request.$case === 'switchHands') value = { removedHandIds: [] };
      else if (request.$case === 'rerollDice') value = { diceToReroll: [] };
      else if (request.$case === 'chooseActive') value = { activeCharacterId: request.value.candidateIds[0] };
      else if (request.$case === 'selectCard') value = { selectedDefinitionId: request.value.candidateDefinitionIds[0] };
      else {
        const valid = request.value.action.map((a, i) => [a, i]).filter(([a]) => a.validity === 0);
        for (const [a] of valid) record.offered.push(a.action.value.cardDefinitionId ?? a.action.value.skillDefinitionId ?? 0);
        const preferred = valid.filter(([a]) => message.who === 0 && (a.action.$case === 'playCard' || a.action.$case === 'useSkill'));
        const candidates = preferred.length && random() < 0.85 ? preferred : valid;
        const [action, index] = candidates[Math.floor(random() * candidates.length)];
        record.chosen.push(action.action.value.cardDefinitionId ?? action.action.value.skillDefinitionId ?? 0);
        if (action.action.$case === "playCard") record.chosenAt.push({ id: action.action.value.cardDefinitionId, afterDecision: decisions });
        value = { chosenActionIndex: index, usedDice: action.autoSelectedDice };
      }
      const response = { $case: request.$case, value };
      messages.push({ in: { type: 'respond', game: id, who: message.who, response } });
      queueMicrotask(() => session.respond(message.who, response));
    };
    session = new GameSession(id, game, emit);
    for (const player of game.players) player.io.notify = collect;
    session.run();
  });
  // Ended/stopped sessions leave only settled microtasks; let those drain before the next fixture.
  await new Promise(resolve => setImmediate(resolve));
  for (const message of messages) gzip.write(JSON.stringify(message) + '\n');
  record.offered = [...new Set(record.offered)]; record.chosen = [...new Set(record.chosen)]; record.triggered = [...new Set(record.triggered)];
  coverage.push(record);
  if (gzip.writableNeedDrain) await once(gzip, 'drain');
}
gzip.end(); await once(output, 'close');
writeFileSync(options.coverage, JSON.stringify({ definitions: definitions.map(d => ({ id: d.id, type: d.type })), games: coverage }, null, 2) + '\n');
console.log(JSON.stringify({ games: coverage.length, seconds: (performance.now() - started) / 1000,
  failures: coverage.filter(r => r.failure).length, output: options.output }));
