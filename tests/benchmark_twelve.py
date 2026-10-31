#!/usr/bin/env python3
"""Launch twelve engine processes with no extra controller, then collect their recorded results.

The launcher becomes the parent engine via exec; all twelve write separate files. The parent runs one shard
and waits for its children. Run --collect after completion. This measures total wall time including startup,
on the same seeded 720-game workload per backend.
"""
import argparse
import json
import os
import pathlib
import time

# Dimbreath's NPC deck table, in GITCG_DATA or the data/ folder of the repository the engine is checked out in.
ENGINE_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DECK_TABLE = os.path.join(os.environ.get('GITCG_DATA', os.path.join(ENGINE_ROOT, '..', 'data')), 'raw', 'GCGDeckExcelConfigData.json')

ROOT = pathlib.Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('backend', choices=('node', 'cpp'))
parser.add_argument('--games', type=int, default=60, help='Games per process')
parser.add_argument('--collect', action='store_true')
parser.add_argument('--directory', default='tests/generated/finish_throughput')
args = parser.parse_args()
directory = ROOT / args.directory / args.backend
processes = 12
if args.games < 1:
    parser.error('--games must be positive')

if args.collect:
    configuration = json.loads((directory / 'configuration.json').read_text())
    shards = []
    for index in range(processes):
        path = directory / f'{index}.log'
        lines = [line for line in path.read_text().splitlines() if line.startswith('{')]
        if not lines:
            raise RuntimeError(f'{path}: engine has not produced its result')
        result = json.loads(lines[-1])
        if result['attempted'] != configuration['gamesPerProcess']:
            raise RuntimeError(f'{path}: incorrect game count')
        shards.append(result)
    seconds = max((directory / f'{index}.log').stat().st_mtime_ns for index in range(processes)) / 1e9 - configuration['started']
    totals = {key: sum(row[key] for row in shards) for key in
              ('attempted', 'finished', 'untranslated', 'different', 'decisions', 'rounds')}
    totals.update(seconds=seconds, gamesPerSecond=totals['finished'] / seconds,
                  decisionsPerSecond=totals['decisions'] / seconds)
    result = {'configuration': configuration, 'summary': totals, 'shards': shards}
    (directory / 'summary.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(totals))
    raise SystemExit(bool(totals['untranslated'] or totals['different']))

directory.mkdir(parents=True, exist_ok=True)
(directory / 'summary.json').unlink(missing_ok=True)
for index in range(processes):
    (directory / f'{index}.log').write_text('')
configuration = {'backend': args.backend, 'processes': processes, 'gamesPerProcess': args.games,
                 'started': time.time(), 'timing': 'wall time including process startup and data loading'}
(directory / 'configuration.json').write_text(json.dumps(configuration, indent=2) + '\n')
os.chdir(ROOT)

# Each backend's parent performs shard 11 itself, then waits for its eleven engine children.
# The Python launcher is replaced, keeping the total at twelve processes throughout the workload.
command = (['node', 'tests/benchmark_parallel.mjs', str(args.games), str(directory)] if args.backend == 'node'
           else ['build/release/benchmark_random_games', str(args.games),
                 str(DECK_TABLE), '0', str(directory)])
os.execvp(command[0], command)
