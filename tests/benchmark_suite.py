#!/usr/bin/env python3
"""Twelve logical throughput shards, at most three engine children plus this driver. No network or installs."""
import os
import argparse
import concurrent.futures
import json
import pathlib
import subprocess
import time

# Dimbreath's NPC deck table, in GITCG_DATA or the data/ folder of the repository the engine is checked out in.
ENGINE_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DECK_TABLE = os.path.join(os.environ.get('GITCG_DATA', os.path.join(ENGINE_ROOT, '..', 'data')), 'raw', 'GCGDeckExcelConfigData.json')

ROOT = pathlib.Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--games', type=int, default=30, help='Games per shard')
parser.add_argument('--shards', type=int, default=12)
parser.add_argument('--jobs', type=int, choices=(1, 2, 3), default=3)
parser.add_argument('--output', default='tests/generated/throughput.json')
args = parser.parse_args()
results = {}
for backend in ('node', 'cpp'):
    started = time.monotonic()
    def run(shard):
        offset = shard * args.games
        if backend == 'node':
            command = ['node', 'tests/benchmark_games.mjs', str(args.games), str(offset)]
        else:
            command = ['build/release/benchmark_random_games', str(args.games),
                       str(DECK_TABLE), str(offset)]
        process = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=600)
        lines = [line for line in process.stdout.splitlines() if line.startswith('{')]
        if not lines:
            raise RuntimeError(f'{backend} shard {shard}: {process.returncode}: {process.stderr}')
        result = json.loads(lines[-1])
        result['shard'] = shard
        result['exitCode'] = process.returncode
        return result
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        shards = list(pool.map(run, range(args.shards)))
    seconds = time.monotonic() - started
    totals = {key: sum(row[key] for row in shards) for key in
              ('attempted', 'finished', 'untranslated', 'different', 'decisions', 'rounds')}
    totals.update(seconds=seconds, gamesPerSecond=totals['finished']/seconds,
                  decisionsPerSecond=totals['decisions']/seconds, engineSeconds=sum(row['seconds'] for row in shards))
    results[backend] = {'summary': totals, 'shards': shards}
    print(backend, json.dumps(totals), flush=True)
results['configuration'] = {'shards': args.shards, 'concurrentChildren': args.jobs,
                          'note': '12 logical shards in batches; a simultaneous 12-process run exceeds the brief cap.'}
(ROOT / args.output).write_text(json.dumps(results, indent=2) + '\n')
