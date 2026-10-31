#!/usr/bin/env python3
"""Run the finish brief's equivalence suites sequentially, with at most three replay children."""
import argparse
import json
import os
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--suites', choices=('core', 'tail', 'all'), default='all')
parser.add_argument('--label', default='final')
parser.add_argument('--slot-coverage', action='store_true')
args = parser.parse_args()
output = ROOT / 'tests/generated' / f'finish_{args.label}'
output.mkdir(parents=True, exist_ok=True)
suites = []
if args.suites in ('core', 'all'):
    suites.extend((f'generated/reference_{index}', count, False) for index, count in ((1, 40), (2, 200)))
    suites.extend((f'fixtures/{name}', count, False) for name, count in
                  (('starter_protocol', 14), ('focused_protocol', 7), ('wide_differences', 6)))
if args.suites in ('tail', 'all'):
    suites.extend((f'generated/{name}', count, snapshots) for name, count, snapshots in
                  (('wide_refreshed', 2400, False), ('sweep_reference', 1546, True), ('sweep_opening_reference', 16, True)))
    suites.append(('fixtures/sweep_difference', 1, True))
failed = False
for source, count, snapshots in suites:
    subprocess.run(['free', '-g'], check=True)
    name = source.split('/')[-1]
    report = output / f'{name}.json'
    command = ['node', 'tests/replay_reference.mjs', f'tests/{source}.jsonl.gz',
               '--jobs', '3', '--report', str(report)] + (['--snapshots'] if snapshots else [])
    if args.slot_coverage:
        command.append('--slot-coverage')
    print(' '.join(command), flush=True)
    with (output / f'{name}.log').open('w') as log:
        result = subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    summary = json.loads(report.read_text())['summary']
    print(name, json.dumps(summary), flush=True)
    failed |= result.returncode != 0 or summary['games'] != count or summary['identical'] != count
if args.suites in ('core', 'all'):
    for program, parameters in (('copy_state_check', ['100']), ('forced_copy_check', []), ('attachment_cost_check', [])):
        subprocess.run(['free', '-g'], check=True)
        environment = {**os.environ, **({'GITCG_TRACE_SLOTS': '1'} if args.slot_coverage else {})}
        result = subprocess.run([f'build/release/{program}', *parameters], cwd=ROOT, capture_output=True, text=True, env=environment)
        (output / f'{program}.log').write_text(result.stdout + result.stderr)
        print(result.stdout + ''.join(line for line in result.stderr.splitlines(keepends=True) if not line.startswith('slot ')), end='', flush=True)
        failed |= result.returncode != 0
        if program == 'copy_state_check':
            failed |= '100/100 unchanged-state restarts identical, 0 untranslated' not in result.stdout
raise SystemExit(bool(failed))
