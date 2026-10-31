// Record Node bridge messages, including random copies and all reply views, through a GameSession adapter.
// refresh_reference.py can re-record the inputs through the actual Node stdio process.
// Each recording group owns its root game and copies, so replay can use independent worker processes.
import { readFileSync, createWriteStream } from 'node:fs';
import { once } from 'node:events';
import { createGzip } from 'node:zlib';
import { parseArgs } from 'node:util';
import { seededRandom } from '../reference/bridge/seeded_random.mjs';
import { pairDecks } from './deck_pairing.mjs';
import { randomResponse } from './random_responder.mjs';
import { startReference } from './reference_client.mjs';

const { values: options } = parseArgs({ options: {
  games: { type: 'string', default: '20' }, seed: { type: 'string', default: '101' },
  copies: { type: 'boolean' }, previews: { type: 'boolean' }, replies: { type: 'string' },
  decisions: { type: 'string', default: '0' }, sources: { type: 'string', default: 'tournament,npc,stage,synergy' },
  output: { type: 'string' }, decks: { type: 'string' },
} });
const seed = Number(options.seed);
const decks = JSON.parse(readFileSync(options.decks ?? new URL('generated/test_decks.json', import.meta.url), 'utf8'));
const path = options.output ?? new URL(`generated/protocol_${seed}.jsonl.gz`, import.meta.url).pathname;
const output = createWriteStream(path), gzip = createGzip(); gzip.pipe(output);
const client = startReference();
let totalDecisions = 0, copyCount = 0, failures = 0, replies = 0;
const modes = options.replies?.split(',') ?? [];
const started = performance.now();
try {
  for (let index = 0; index < Number(options.games); ++index) {
    const random = seededRandom(seed * 1000003 + index);
    const sources = options.sources.split(',');
    const pair = pairDecks(decks, [sources[index % sources.length]], random);
    const mode = modes[index % modes.length];
    const view = (seat) => mode === 'none' ? null : ({
      opponentDeck: mode === 'onDemand' ? 'guessed' : mode === 'emptyGuess' ? 'guessed' : mode === 'oneSeat' ? 'hidden' : mode,
      ...((mode === 'guessed' || mode === 'onDemand') ? { candidates: [[4, pair.decks[1 - seat].cards], [0, pair.decks[seat].cards]] } : {}),
      ...(mode === 'emptyGuess' ? { candidates: [] } : {}),
      ...(mode === 'onDemand' ? { onDemand: true } : {}),
    });
    const create = { type: 'create', game: index, decks: pair.decks, equipment: pair.equipment,
      seed: Math.floor(random() * 2 ** 31), privileged: index % 2 === 0,
      ...(options.previews ? { previews: true } : {}),
      ...(mode ? { replies: true, replyViews: [view(0), mode === 'oneSeat' ? null : view(1)] } : {}),
    };
    gzip.write(JSON.stringify({ create, origins: pair.origins, mode }) + '\n');
    client.send(create);
    const active = new Set([index]), counts = new Map(), awaitingReplies = new Map();
    let madeCopies = 0, deferredAnswer = null;
    const send = message => { gzip.write(JSON.stringify({ in: message }) + '\n'); client.send(message); };
    while (active.size) {
      const message = await client.next(120000);
      gzip.write(JSON.stringify({ out: message }) + '\n');
      if (message.type === 'decision') {
        totalDecisions++;
        const count = (counts.get(message.game) ?? 0) + 1; counts.set(message.game, count);
        if (message.request.$case === 'action' && ((message.game !== index && count >= 5) || (Number(options.decisions) && count >= Number(options.decisions)))) {
          send({ type: 'stop', game: message.game }); active.delete(message.game);
          if (active.size === 1 && active.has(index) && deferredAnswer) { send(deferredAnswer); deferredAnswer = null; }
          continue;
        }
        const response = randomResponse(message, random);
        const answer = { type: 'respond', game: message.game, who: message.who, response };
        if (options.copies && !message.repliesOnDemand && message.game === index && response.$case === 'action' && madeCopies < 3 && random() < 0.25) {
          const id = 1000000 + index * 10 + madeCopies++;
          const copy = { type: 'copy', game: index, copy: id, who: message.who, response, seed: Math.floor(random() * 2 ** 31) };
          if (madeCopies === 2) { // Deliberately force divergence by giving a different seat its first answer.
            copy.who = 1 - message.who;
          }
          if (madeCopies === 3) {
            const opponent = message.state.player[1 - message.who];
            const count = opponent.handCard.length + opponent.pileCard.length;
            copy.opponentCards = Array.from({ length: count }, (_, i) => pair.decks[1 - message.who].cards[i % pair.decks[1 - message.who].cards.length]);
          }
          active.add(id); copyCount++; send(copy); deferredAnswer = answer;
        }
        if (message.repliesOnDemand) {
          const indices = message.request.value.action.flatMap((action, i) => action.validity === 0 ? [i] : []);
          const selected = indices.filter(() => random() < 0.2).slice(0, 3);
          if (!selected.includes(response.value.chosenActionIndex)) selected.push(response.value.chosenActionIndex);
          // Repeated indices verify Set order and deduplication.
          awaitingReplies.set(`${message.game}:${message.who}`, answer);
          send({ type: 'replies', game: message.game, who: message.who, indices: [...selected, selected[0]] });
        } else if (deferredAnswer !== answer) send(answer);
      } else if (message.type === 'replies') {
        replies++;
        const key = `${message.game}:${message.who}`;
        const answer = awaitingReplies.get(key); awaitingReplies.delete(key); send(answer);
      } else if (message.type === 'finished' || message.type === 'failed') {
        if (message.type === 'failed') failures++;
        active.delete(message.game);
        if (active.size === 1 && active.has(index) && deferredAnswer) { send(deferredAnswer); deferredAnswer = null; }
      } else throw new Error(`unexpected worker message ${JSON.stringify(message)}`);
    }
    if (gzip.writableNeedDrain) await once(gzip, 'drain');
    await new Promise(resolve => setImmediate(resolve));
  }
} finally { await client.close(); gzip.end(); await once(output, 'close'); }
console.log(JSON.stringify({ games: Number(options.games), copies: copyCount, decisions: totalDecisions, replies, failures,
  seconds: (performance.now() - started) / 1000, output: path }));
