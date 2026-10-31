#!/usr/bin/env python3
"""Replay recorded inputs through the actual Node stdio worker and re-record its outputs.

Each input gets a synchronous error-probe acknowledgement before the next input can be sent. This keeps
readline from processing two real inputs in one callback, before Node drains its promise reactions. Pending
decision snapshots then use the same event boundary as reference_client.mjs and the C++ worker. This checks
message envelopes and terminal state, and never reads C++ output to construct an oracle.
"""
import argparse
import gzip
import json
import pathlib
import queue
import subprocess
import threading

ROOT = pathlib.Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('source')
parser.add_argument('output')
parser.add_argument('--indices', help='Comma-separated groups to re-record; default all')
args = parser.parse_args()
wanted = set(map(int, args.indices.split(','))) if args.indices else None
process = subprocess.Popen(['node', 'reference/bridge/worker.mjs'], cwd=ROOT, stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=1)
messages = queue.Queue()
acknowledgements = queue.Queue()
probe_game = -2147483647
def read():
    for line in process.stdout:
        message = json.loads(line)
        if message.get('type') == 'failed' and message.get('game') == probe_game:
            acknowledgements.put(message)
        else:
            messages.put(message)
    messages.put({'type': 'worker ended'})
threading.Thread(target=read, daemon=True).start()
def send(value=None):
    if value and (value.get('game') == probe_game or value.get('copy') == probe_game):
        raise ValueError('recording uses the reserved probe game id')
    probe = {'type': 'respond', 'game': probe_game, 'who': 0, 'response': {}}
    process.stdin.write(('' if value is None else json.dumps(value) + '\n') + json.dumps(probe) + '\n')
    process.stdin.flush()
    acknowledgements.get(timeout=60)
count = changed = 0
try:
    with gzip.open(args.source, 'rt') as source, gzip.open(args.output, 'wt', compresslevel=1) as output:
        def group(lines, index):
            global count, changed
            if wanted is not None and index not in wanted:
                output.writelines(lines); return
            count += 1
            for line in lines:
                entry = json.loads(line)
                value = entry.get('create') or entry.get('in')
                if value:
                    send(value); output.write(line); continue
                expected = entry['out']
                actual = messages.get(timeout=60)
                envelope = ('type', 'game', 'who')
                if any(expected.get(key) != actual.get(key) for key in envelope):
                    raise RuntimeError(f'group {index}: envelope differs: {expected.get("type")} vs {actual}')
                changed += expected != actual
                output.write(json.dumps({'out': actual}, separators=(',', ':')) + '\n')
            # The final input was acknowledged after every earlier synchronous output. Async decisions are
            # consumed above; another probe in its own callback drains their promise reactions as well.
            send()
            if not messages.empty():
                raise RuntimeError(f'group {index}: unexpected extra output {messages.get_nowait()}')
        current, index = [], -1
        for line in source:
            if line.startswith('{"create"'):
                if current: group(current, index)
                current = []; index += 1
            current.append(line)
        if current: group(current, index)
    process.stdin.write(json.dumps({'type': 'shutdown'}) + '\n'); process.stdin.flush()
    process.wait(timeout=10)
finally:
    if process.poll() is None: process.kill(); process.wait()
print(json.dumps({'groupsRefreshed': count, 'outputsChanged': changed, 'output': args.output}))
