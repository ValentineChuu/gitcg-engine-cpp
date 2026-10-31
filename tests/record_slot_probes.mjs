// Record targeted Node games from explicit controlled states; replay with --snapshots.
import { createWriteStream, readFileSync, writeFileSync } from 'node:fs';
import { once } from 'node:events';
import { createGzip } from 'node:zlib';
import { parseArgs } from 'node:util';
import getData from '@gi-tcg/data';
import { GameSession } from '../reference/bridge/game_session.mjs';
import { seededRandom } from '../reference/bridge/seeded_random.mjs';
import { randomResponse } from './random_responder.mjs';
import { snapshotState } from './state_snapshot.mjs';
import { makeProbe } from './slot_probe_setup.mjs';

const { values: options } = parseArgs({ options: {
  cases: { type: 'string', default: 'tests/generated/coverage_probe_cases.json' },
  output: { type: 'string', default: 'tests/generated/coverage_probe_reference.jsonl.gz' },
  seed: { type: 'string', default: '706' },
} });
const probes = JSON.parse(readFileSync(options.cases));
const data = getData();
const output = createWriteStream(options.output), gzip = createGzip(); gzip.pipe(output);
const outcomes = [];
for (const [index, probe] of probes.entries()) {
  const seed = Number(options.seed) * 1000003 + index, random = seededRandom(seed);
  const { game, decks } = makeProbe(data, probe, seed);
  const create = { type: 'create', game: index, seed, decks, initialState: snapshotState(game.state) };
  const messages = [{ create, origins: [`slot-probe:${probe.definition}:${probe.name ?? index}`] }];
  let failure = null, decisions = 0;
  await new Promise(resolve => {
    let session;
    const emit = message => {
      messages.push({ out: message });
      if (message.type === 'failed' || message.type === 'finished') {
        failure = message.type === 'failed' ? message.message : null;
        resolve(); return;
      }
      if (message.type !== 'decision') throw new Error(`Unexpected ${message.type}`);
      decisions++;
      if (decisions >= (probe.decisions ?? 140) || message.state.roundNumber >= (probe.rounds ?? 6)) {
        messages.push({ in: { type: 'stop', game: index } });
        session.stop(); resolve(); return;
      }
      let response = randomResponse(message, random);
      if (message.request.$case === 'action') {
        const valid = message.request.value.action.map((action, index) => [action, index]).filter(([action]) => (action.validity ?? 0) === 0);
        const preferred = valid.filter(([action]) => ['playCard', 'useSkill'].includes(action.action.$case));
        const candidates = preferred.length && random() < 0.95 ? preferred : valid;
        let [action, chosenActionIndex] = candidates[Math.floor(random() * candidates.length)];
        const forced = (probe.prefer ?? []).map(id => valid.find(([action]) => id === (action.action.value.skillDefinitionId ?? action.action.value.cardDefinitionId))).find(Boolean);
        if (forced) [action, chosenActionIndex] = forced;
        if (message.who === (probe.switchWho ?? 0) && probe.switchTo) {
          const switching = valid.find(([action]) => action.action.$case === 'switchActive' && action.action.value.characterDefinitionId === probe.switchTo);
          if (switching) [action, chosenActionIndex] = switching;
        }
        response = { $case: 'action', value: { chosenActionIndex, usedDice: action.autoSelectedDice } };
      }
      messages.push({ in: { type: 'respond', game: index, who: message.who, response } });
      queueMicrotask(() => session.respond(message.who, response));
    };
    session = new GameSession(index, game, emit);
    session.run();
  });
  await new Promise(resolve => setImmediate(resolve));
  for (const message of messages) if (!gzip.write(JSON.stringify(message) + '\n')) await once(gzip, 'drain');
  outcomes.push({ index, definition: probe.definition, name: probe.name, decisions, failure });
}
gzip.end(); await once(output, 'close');
writeFileSync(options.output + '.outcomes.json', JSON.stringify(outcomes, null, 2) + '\n');
console.log(JSON.stringify({ games: probes.length, failures: outcomes.filter(result => result.failure).length,
  decisions: outcomes.reduce((sum, result) => sum + result.decisions, 0), output: options.output }));
