// One complete, seeded Node game per supported stage, against a legal collection deck.
import { createWriteStream, readFileSync } from 'node:fs';
import { once } from 'node:events';
import { createGzip } from 'node:zlib';
import { parseArgs } from 'node:util';
import getData from '@gi-tcg/data';
import { GameSession } from '../reference/bridge/game_session.mjs';
import { seededRandom } from '../reference/bridge/seeded_random.mjs';
import { randomResponse } from './random_responder.mjs';

const { values: options } = parseArgs({ options: {
  seed: { type: 'string', default: '704' },
  output: { type: 'string', default: 'tests/generated/coverage_stage_reference.jsonl.gz' },
} });
const { stages } = JSON.parse(readFileSync(new URL('generated/coverage_stages.json', import.meta.url)));
const data = getData();
const output = createWriteStream(options.output), gzip = createGzip();
gzip.pipe(output);
let failures = 0, decisions = 0;
for (const [index, stage] of stages.entries()) {
  const seed = Number(options.seed) * 1000003 + index, random = seededRandom(seed);
  const create = { type: 'create', game: index, seed, decks: stage.decks, equipment: stage.equipment };
  const messages = [{ create, origins: [`collection:${stage.collection}`, `stage:${stage.levelId}:${stage.kind}:${stage.name}`] }];
  await new Promise(resolve => {
    let session;
    const emit = message => {
      messages.push({ out: message });
      if (message.type === 'finished' || message.type === 'failed') {
        failures += message.type === 'failed';
        resolve();
      } else if (message.type === 'decision') {
        decisions++;
        let response = randomResponse(message, random);
        if (message.request.$case === 'action' && random() < 0.9) {
          const preferred = message.request.value.action.map((action, index) => [action, index])
            .filter(([action]) => (action.validity ?? 0) === 0 && ['useSkill', 'playCard'].includes(action.action.$case));
          if (preferred.length) {
            const [action, chosenActionIndex] = preferred[Math.floor(random() * preferred.length)];
            response = { $case: 'action', value: { chosenActionIndex, usedDice: action.autoSelectedDice } };
          }
        }
        messages.push({ in: { type: 'respond', game: index, who: message.who, response } });
        queueMicrotask(() => session.respond(message.who, response));
      }
    };
    session = GameSession.create(index, stage.decks, seed, data, emit, stage.equipment);
    session.run();
  });
  await new Promise(resolve => setImmediate(resolve));
  for (const message of messages) if (!gzip.write(JSON.stringify(message) + '\n')) await once(gzip, 'drain');
  if ((index + 1) % 100 === 0) console.log(JSON.stringify({ games: index + 1, failures, decisions }));
}
gzip.end();
await once(output, 'close');
console.log(JSON.stringify({ games: stages.length, failures, decisions, output: options.output }));
