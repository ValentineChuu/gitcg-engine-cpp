// A bounded, reusable stdio worker client for equivalence tests and benchmarks.
import { spawn } from 'node:child_process';
import { createInterface } from 'node:readline';

export function startWorker(command = new URL('../build/release/worker', import.meta.url).pathname, args = []) {
  const worker = spawn(command, args, { stdio: ['pipe', 'pipe', 'pipe'] });
  const queue = [];
  let waiter, ended, stderr = '', traceLine = '';
  const slotCounts = new Map();
  worker.stderr.on('data', chunk => {
    stderr = (stderr + chunk).slice(-16000);
    // Count complete trace lines as they arrive; the bounded diagnostic tail loses early calls.
    const lines = (traceLine + chunk).split('\n');
    traceLine = lines.pop();
    for (const line of lines) {
      const match = /^slot (\d+) after message \d+$/.exec(line);
      if (match) {
        const slot = Number(match[1]);
        slotCounts.set(slot, (slotCounts.get(slot) ?? 0) + 1);
      }
    }
  });
  const deliver = message => {
    if (waiter) { const pending = waiter; waiter = null; clearTimeout(pending.timer); pending.resolve(message); }
    else queue.push(message);
  };
  createInterface({ input: worker.stdout }).on('line', line => {
    try { deliver(JSON.parse(line)); } catch { deliver({ type: 'invalid output', line }); }
  });
  worker.on('error', error => { ended = { type: 'worker ended', reason: error.message }; deliver(ended); });
  worker.on('exit', (code, signal) => {
    ended = { type: 'worker ended', reason: signal ?? `exit code ${code}`, stderr };
    if (waiter) deliver(ended);
  });
  return {
    worker, slotCounts, get stderr() { return stderr; },
    send(message) { worker.stdin.write(JSON.stringify(message) + '\n'); },
    next(timeout = 60000) {
      if (queue.length) return Promise.resolve(queue.shift());
      if (ended) return Promise.resolve(ended);
      return new Promise((resolve, reject) => {
        waiter = { resolve, timer: setTimeout(() => { waiter = null; reject(new Error('worker timeout')); }, timeout) };
      });
    },
    async close() {
      if (waiter) { clearTimeout(waiter.timer); waiter.resolve({ type: 'worker closed' }); waiter = null; }
      if (ended) return;
      await new Promise(resolve => { worker.once('exit', resolve); worker.kill(); });
    },
  };
}

export function firstDifference(expected, got, path = '') {
  if (expected === got) return null;
  if (expected === null || got === null || typeof expected !== typeof got || Array.isArray(expected) !== Array.isArray(got))
    return { path, expected, got };
  if (typeof expected === 'object') {
    for (const key of new Set([...Object.keys(expected), ...Object.keys(got)])) {
      const difference = firstDifference(expected[key], got[key], `${path}.${key}`);
      if (difference) return difference;
    }
    return null;
  }
  return { path, expected, got };
}
