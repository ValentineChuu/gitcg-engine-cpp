// Replay reference traces, stopping each game at its first difference. A driver and at most three workers obey
// the shared-PC four-process limit. Failed messages are compared by kind, never by language-specific stack text.
import { createReadStream, writeFileSync } from 'node:fs';
import { createInterface } from 'node:readline';
import { createGunzip } from 'node:zlib';
import { parseArgs } from 'node:util';
import { startWorker, firstDifference } from './worker_client.mjs';

const { values: options, positionals } = parseArgs({ allowPositionals: true, options: {
  games: { type: 'string' }, only: { type: 'string' }, worker: { type: 'string' },
  snapshots: { type: 'boolean' }, jobs: { type: 'string', default: '3' }, report: { type: 'string' }, verbose: { type: 'boolean', default: false },
  'slot-coverage': { type: 'boolean', default: false },
} });
if (options['slot-coverage']) process.env.GITCG_TRACE_SLOTS = '1';
const jobs = Number(options.jobs);
if (!Number.isInteger(jobs) || jobs < 1 || jobs > 3) throw new Error('--jobs must be 1, 2 or 3 (four processes including the driver)');
for (const key of ['games', 'only']) if (options[key] !== undefined &&
  (!Number.isInteger(Number(options[key])) || Number(options[key]) < (key === 'games' ? 1 : 0))) throw new Error(`Invalid --${key}`);
const path = positionals[0] ?? new URL('generated/reference_1.jsonl.gz', import.meta.url).pathname;
async function* readGames() {
  const input = createInterface({ input: createReadStream(path).pipe(createGunzip()), crlfDelay: Infinity });
  let game;
  for await (const line of input) {
    if (!line) continue;
    const entry = JSON.parse(line);
    if (entry.create) {
      if (game) yield game;
      game = { create: entry.create, origins: entry.origins, steps: [] };
    } else game.steps.push(entry);
  }
  if (game) yield game;
}
const results = [];

const started = performance.now();
async function replay(index, game) {
  const client = startWorker(options.worker, options.snapshots ? ['--test-snapshots'] : []);
  let decisions = 0, messages = 0, result;
  try {
    client.send(game.create);
    for (const step of game.steps) {
      if (step.in) { client.send(step.in); continue; }
      const got = await client.next();
      const slot = got.type === 'failed' && /lambda slot (\d+) not translated/.exec(got.message);
      if (slot) { result = { status: 'untranslated', slot: Number(slot[1]), failure: got.message }; break; }
      const difference = step.out.type === 'failed' && got.type === 'failed' && step.out.game === got.game
        ? null : firstDifference(step.out, got);
      if (difference) {
        result = { status: 'different', ...difference, expectedType: step.out.type, gotType: got.type,
                   ...(got.type === 'failed' ? { failure: got.message } : got.type === 'worker ended' ? { failure: got.reason, stderr: got.stderr } : {}) };
        break;
      }
      messages++;
      if (got.type === 'decision') decisions++;
    }
    // Recordings can intentionally stop games/copies. A final ordered error probe catches unexpected extra output.
    if (!result) {
      client.send({ type: 'respond', game: -2147483647, who: 0, response: {} });
      const extra = await client.next();
      if (extra.type !== 'failed' || extra.game !== -2147483647)
        result = { status: 'different', path: 'extra output', got: extra };
    }
  } catch (error) { result = { status: 'different', failure: error.message }; }
  finally { await client.close(); }
  result = { index, origins: game.origins, decisions, messages, ...(result ?? { status: 'identical' }),
             ...(options['slot-coverage'] ? { slotCounts: Object.fromEntries([...client.slotCounts].sort((a, b) => a[0] - b[0])) } : {}) };
  if (result.status === 'different' || options.verbose) console.log(JSON.stringify(result));
  results.push(result);
}
const pending = new Set();
let gameIndex = 0;
for await (const game of readGames()) {
  const index = gameIndex++;
  if (options.only !== undefined && index !== Number(options.only)) continue;
  if (options.games !== undefined && index >= Number(options.games)) break;
  const task = replay(index, game).finally(() => pending.delete(task));
  pending.add(task);
  if (pending.size >= jobs) await Promise.race(pending);
  if (options.only !== undefined) break;
}
await Promise.all(pending);
if (!results.length) throw new Error('No games selected');
results.sort((a, b) => a.index - b.index);
const count = status => results.filter(result => result.status === status).length;
const summary = { games: results.length, identical: count('identical'), untranslated: count('untranslated'),
                  different: count('different'), decisions: results.reduce((n, r) => n + r.decisions, 0),
                  seconds: (performance.now() - started) / 1000 };
console.log(JSON.stringify(summary));
if (options.report) writeFileSync(options.report, JSON.stringify({ summary, results }, null, 2) + '\n');
process.exitCode = summary.different ? 1 : 0;
