#!/usr/bin/env python3
"""Inventory unbound slots after translator output and all hand overrides (including extension hooks)."""
import collections
import json
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[2]
slots = json.loads((ROOT / 'cards/generated/lambda_slots.json').read_text())
reasons = {entry['id']: entry['reasons'] for entry in json.loads((ROOT / 'cards/translated/untranslated.json').read_text())}
translated = set()
manual = set()
for directory, bound in (('translated', translated), ('manual', manual)):
    for path in (ROOT / 'cards' / directory).rglob('*.cpp'):
        source = path.read_text()
        bound.update(map(int, re.findall(r'slots\.bind\((\d+),', source)))
        loops = re.findall(r'for \(int slot : \{([\d, ]+)\}\) \{\s*slots\.bind\(slot,', source)
        if len(loops) != len(re.findall(r'slots\.bind\(slot,', source)):
            raise ValueError(f'{path}: unsupported dynamic binding; extend the audit')
        for loop in loops:
            bound.update(map(int, loop.split(',')))
        if re.search(r'slots\.bind\((?!\d|slot,)', source):
            raise ValueError(f'{path}: unknown binding shape')
unbound = [{'id': slot['id'], 'file': slot['file'], 'line': slot['gts']['line'], 'kind': slot['kind'],
            'reasons': reasons.get(slot['id'], ['missing generated binding'])} for slot in slots
           if slot['id'] not in translated | manual]
summary = {'total': len(slots), 'translated': len(translated), 'manual': len(manual),
           'manualOnly': len(manual - translated), 'bound': len(translated | manual), 'unbound': len(unbound),
           'unboundByDirectory': dict(collections.Counter(entry['file'].split('/')[0] for entry in unbound))}
output = ROOT / 'docs/finish_untranslated.json'
output.write_text(json.dumps({'summary': summary, 'slots': unbound}, indent=2) + '\n')
print(json.dumps(summary))
