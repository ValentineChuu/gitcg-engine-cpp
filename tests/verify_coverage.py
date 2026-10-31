#!/usr/bin/env python3
"""Replay newly recorded coverage suites with traces, sequentially and with three children."""
import argparse
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--sources', nargs='+', default=['stage', 'sweep', 'probe', 'focused', 'regressions'])
args = parser.parse_args()
output = ROOT / 'tests/generated/coverage_replays'
output.mkdir(exist_ok=True)
failed = False
for source in args.sources:
    subprocess.run(['free', '-g'], check=True)
    recording = ROOT / f'tests/generated/coverage_{source}_reference.jsonl.gz'
    if source == 'regressions':
        recording = ROOT / 'tests/fixtures/coverage_regressions.jsonl.gz'
    report = output / f'{source}.json'
    command = ['node', 'tests/replay_reference.mjs', str(recording), '--jobs', '3',
               '--slot-coverage', '--report', str(report)]
    if source != 'stage':
        command.append('--snapshots')
    with (output / f'{source}.log').open('w') as log:
        result = subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    replay = json.loads(report.read_text())
    summary = replay['summary']
    print(source, json.dumps(summary), flush=True)
    expected = {'stage': 1162, 'sweep': 1546, 'probe': 198, 'focused': 42, 'regressions': 4}.get(source)
    failed |= result.returncode != 0 or summary['games'] != summary['identical']
    if expected is not None:
        failed |= summary['games'] != expected
    if source == 'regressions':
        required = json.loads(recording.with_name('coverage_regressions.json').read_text())['requiredSlots']
        executed = {int(slot) for result in replay['results'] for slot in result['slotCounts']}
        failed |= not set(required) <= executed
raise SystemExit(bool(failed))
