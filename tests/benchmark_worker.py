#!/usr/bin/env python3
"""Actual stdio decisions/s, with replies eager or on demand; one engine child at a time on each backend."""
import argparse
import hashlib
import json
import math
import os
import pathlib
import queue
import subprocess
import threading
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--games', type=int, default=8)
parser.add_argument('--deck', choices=('simple', 'focused'), default='simple')
parser.add_argument('--output', default='tests/generated/worker_speed.json')
args = parser.parse_args()
deck = json.loads((ROOT / f'tests/fixtures/{args.deck}_decks.json').read_text())[0]

class Random:
    def __init__(self, seed): self.value = float(seed % 2**31)
    def next(self):
        self.value = math.fmod(self.value * 1103515245.0 + 12345.0, 2**31)
        return self.value / 2**31

results = []
for mode in ('plain', 'replies', 'onDemand'):
    for backend in ('node', 'cpp'):
        command = ['node', 'reference/bridge/worker.mjs'] if backend == 'node' else ['build/release/worker']
        process = subprocess.Popen(command, cwd=ROOT, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   text=True, env={**os.environ, 'AUTOGITCG_ENGINE_WORK': '1'}, bufsize=1)
        messages = queue.Queue()
        def read_messages():
            for line in process.stdout:
                try: messages.put(json.loads(line))
                except ValueError: messages.put({'type': 'invalid', 'line': line})
            messages.put({'type': 'worker ended'})
        reader = threading.Thread(target=read_messages, daemon=True); reader.start()
        def send(value): process.stdin.write(json.dumps(value) + '\n'); process.stdin.flush()
        finished = untranslated = different = decisions = actions = replies = 0
        hashes, gameplay = [], []
        started = time.monotonic()
        try:
            for index in range(-1, args.games):
                if index == 0:
                    finished = untranslated = different = decisions = actions = replies = 0
                    started = time.monotonic()
                random = Random(7001 + index)
                create = {'type': 'create', 'game': index, 'decks': [deck, deck], 'seed': 7919 * (index + 2)}
                if mode != 'plain':
                    view = {'opponentDeck': 'open', **({'onDemand': True} if mode == 'onDemand' else {})}
                    create.update(replies=True, replyViews=[view, view])
                digest, game_digest = hashlib.sha256(), hashlib.sha256()
                send(create)
                while True:
                    message = messages.get(timeout=120)
                    digest.update(json.dumps(message, sort_keys=True, separators=(',', ':')).encode())
                    kind = message['type']
                    if kind == 'decision':
                        decisions += 1
                        who, request = message['who'], message['request']
                        case, value = request['$case'], request['value']
                        if case == 'action':
                            actions += 1
                            valid = [(i, a) for i, a in enumerate(value['action']) if a.get('validity', 0) == 0]
                            chosen, action = valid[int(random.next() * len(valid))]
                            answer = {'chosenActionIndex': chosen, 'usedDice': action['autoSelectedDice']}
                        elif case == 'chooseActive': answer = {'activeCharacterId': value['candidateIds'][0]}
                        elif case == 'selectCard': answer = {'selectedDefinitionId': value['candidateDefinitionIds'][0]}
                        elif case == 'rerollDice': answer = {'diceToReroll': []}
                        elif case == 'switchHands': answer = {'removedHandIds': []}
                        else: raise RuntimeError(case)
                        # Public gameplay hash excludes optional preview information and search fields.
                        gameplay_request = json.loads(json.dumps(request))
                        if case == 'action':
                            for a in gameplay_request['value']['action']:
                                a.pop('preview', None)
                                a['action']['value'].pop('mainDamageTargetId', None)
                        game_digest.update(json.dumps([who, gameplay_request, message['state']], sort_keys=True).encode())
                        if message.get('repliesOnDemand'):
                            indices = list(dict.fromkeys([*(i for i, _ in valid[:2]), chosen]))
                            send({'type': 'replies', 'game': index, 'who': who, 'indices': indices})
                            reply = messages.get(timeout=120)
                            if reply['type'] != 'replies':
                                if reply['type'] == 'failed' and 'not translated' in reply.get('message', ''): untranslated += 1; break
                                raise RuntimeError(reply)
                            replies += 1
                            digest.update(json.dumps(reply, sort_keys=True, separators=(',', ':')).encode())
                        send({'type': 'respond', 'game': index, 'who': who, 'response': {'$case': case, 'value': answer}})
                    elif kind == 'finished': finished += 1; break
                    elif kind == 'failed':
                        if 'not translated' in message.get('message', ''): untranslated += 1
                        else: different += 1
                        break
                    else: raise RuntimeError(message)
                if index >= 0: hashes.append(digest.hexdigest()); gameplay.append(game_digest.hexdigest())
            seconds = time.monotonic() - started
            send({'type': 'shutdown'}); process.wait(timeout=10); reader.join(timeout=1)
            stderr = process.stderr.read()
        finally:
            if process.poll() is None: process.kill(); process.wait()
            for pipe in (process.stdin, process.stdout, process.stderr): pipe.close()
        work = next((json.loads(line[len('ENGINE_WORK '):]) for line in stderr.splitlines() if line.startswith('ENGINE_WORK ')), None)
        result = dict(backend=backend, mode=mode, attempted=args.games, finished=finished, untranslated=untranslated,
                      different=different, decisions=decisions, actions=actions, replies=replies, seconds=seconds,
                      decisionsPerSecond=decisions/seconds, actionsPerSecond=actions/seconds, hashes=hashes,
                      gameplayHashes=gameplay, engineWork=work)
        results.append(result)
        print(json.dumps({k: v for k, v in result.items() if k not in ('hashes', 'gameplayHashes')}), flush=True)
(ROOT / args.output).write_text(json.dumps(results, indent=2) + '\n')
for mode in ('plain', 'replies', 'onDemand'):
    node, cpp = [row for row in results if row['mode'] == mode]
    if not cpp['untranslated'] and (node['hashes'] != cpp['hashes'] or node['engineWork'] != cpp['engineWork']):
        raise RuntimeError(f'{mode}: workers differ; inspect hashes and work counts')
for backend in ('node', 'cpp'):
    rows = [row for row in results if row['backend'] == backend]
    if all(not row['untranslated'] for row in rows) and any(row['gameplayHashes'] != rows[0]['gameplayHashes'] for row in rows):
        raise RuntimeError(f'{backend}: previews/replies changed gameplay')
(ROOT / args.output).write_text(json.dumps(results, indent=2) + '\n')
