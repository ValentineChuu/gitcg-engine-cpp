#!/usr/bin/env python3
"""Audit binding reachability and merge measured slot calls from replay reports."""
import argparse
import collections
import json
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--reports', nargs='*', default=[])
parser.add_argument('--native-logs', nargs='*', default=[])
parser.add_argument('--baseline')
parser.add_argument('--require-classified', action='store_true')
parser.add_argument('--require-identical', action='store_true')
parser.add_argument('--output', default='docs/coverage_inventory.json')
args = parser.parse_args()
slots = json.loads((ROOT / 'cards/generated/lambda_slots.json').read_text())
translated, manual = set(), set()
for directory, bound in (('translated', translated), ('manual', manual)):
    for path in (ROOT / 'cards' / directory).rglob('*.cpp'):
        source = path.read_text()
        bound.update(map(int, re.findall(r'slots\.bind\((\d+),', source)))
        loops = re.findall(r'for \(int slot : \{([\d, ]+)\}\) \{\s*slots\.bind\(slot,', source)
        if len(loops) != len(re.findall(r'slots\.bind\(slot,', source)):
            raise ValueError(f'{path}: unsupported dynamic binding')
        for loop in loops:
            bound.update(map(int, loop.split(',')))
        if re.search(r'slots\.bind\((?!\d|slot,)', source):
            raise ValueError(f'{path}: unknown binding shape')
bound = translated | manual

# These module helpers/snippets are evaluated or inlined by the listed C++ callers.
# All references were checked against the GTS and the corresponding hand/generated code.
helpers = {
    857: ([733, 734], 'getMelusineEventCards: hand callers implement registry filtering'),
    861: ([830, 1312, 1313], 'SIMULANKA_QUERY: union expanded in hand callers'),
    1093: ([1053], 'PUCAS_ALLIES: module list inlined by the translator'),
    2175: ([2177, 2179], 'addVitality snippet: hand callers inline the state argument'),
    2210: ([2212, 2214, 2215], 'Kinich snippet: all three hand callers inline the state argument'),
    4652: ([4650], 'handlesDisguise: first-alive lookup inlined in hand caller'),
    4993: ([4988, 4991], 'findAbsorbed: absorption table lookup in hand callers'),
    4994: ([4991, 4992], 'absorptionStatusFor: absorption table lookup in hand callers'),
}

def ancestors(slot):
    return sorted((other for other in slots if other['file'] == slot['file'] and other['id'] != slot['id']
                   and other['generated']['offset'] <= slot['generated']['offset']
                   and other['generated']['end'] >= slot['generated']['end']),
                  key=lambda item: item['generated']['end'] - item['generated']['offset'])

def classification(slot):
    if slot['file'].startswith('old_versions/'):
        return {'classification': 'unreachable_old_version', 'why': 'Workers create current v7.1.0 games only.'}
    enclosing = ancestors(slot)
    parent = next((item for item in enclosing if item['id'] in bound), None)
    if parent:
        return {'classification': 'unreachable_inner_slot', 'callers': [parent['id']],
                'why': 'The enclosing bound lambda compiles this expression directly; no slot dispatch.'}
    helper = next((helpers[item['id']] for item in [slot, *enclosing] if item['id'] in helpers), None)
    if helper:
        callers, why = helper
        assert all(caller in bound for caller in callers)
        return {'classification': 'unreachable_helper_slot', 'callers': callers, 'why': why}
    return {'classification': 'reachable', 'why': 'No bound enclosing lambda or fully replaced helper.'}

owners = collections.defaultdict(list)
def references(value, root, path=()):
    if isinstance(value, dict):
        if '$lambda' in value:
            owners[value['$lambda']].append({**root, 'path': list(path)})
        for child in value.get('attributes', []):
            label = child['name']
            if label == 'on':
                label += ' ' + str(child['positionals'][0])
            conditions = [entry['positionals'][0]['$lambda'] for entry in child.get('attributes', [])
                          if entry['name'] == 'when' and isinstance(entry['positionals'][0], dict)
                          and '$lambda' in entry['positionals'][0]]
            references(child, {**root, **({'conditionSlots': conditions} if conditions else {})}, (*path, label))
        for child in value.get('positionals', []):
            references(child, root, path)
    elif isinstance(value, list):
        for child in value:
            references(child, root, path)

for path in (ROOT / 'cards/generated').rglob('*.cpp'):
    match = re.search(r'R"GTS\((.*?)\)GTS"', path.read_text(), re.S)
    if not match:
        continue
    for node in json.loads(match[1]):
        attributes = node.get('attributes', [])
        identifier = next((attr['positionals'][0] for attr in attributes if attr['name'] in ('id', 'idHint')), None)
        references(node, {'definition': identifier, 'type': node['name']})

calls, report_counts, first_execution = collections.Counter(), [], {}
for pattern in args.reports:
    for path in sorted(ROOT.glob(pattern)):
        report = json.loads(path.read_text())
        if 'results' not in report:
            continue
        if not all('slotCounts' in result for result in report['results']):
            raise ValueError(f'{path}: missing --slot-coverage')
        for result in report['results']:
            calls.update({int(slot): count for slot, count in result['slotCounts'].items()})
            for slot, count in result['slotCounts'].items():
                first_execution.setdefault(int(slot), {'report': str(path.relative_to(ROOT)), 'index': result['index'],
                                                      'origins': result.get('origins'), 'calls': count})
        report_counts.append({'path': str(path.relative_to(ROOT)), **report['summary']})
if args.require_identical and any(report['games'] != report['identical'] for report in report_counts):
    raise SystemExit('A replay report contains untranslated stops or differences.')
for pattern in args.native_logs:
    for path in sorted(ROOT.glob(pattern)):
        calls.update(map(int, re.findall(r'^slot (\d+) after message \d+$', path.read_text(), re.M)))

unbound = [{**slot, **classification(slot)} for slot in slots if slot['id'] not in bound]
context = [slot for slot in slots if slot['id'] in bound and slot['kind'] == 'function'
           and slot['source'].startswith('(__gts_fnArg)') and not slot['file'].startswith('old_versions/')]
never = [{**slot, 'owners': owners[slot['id']],
          'binding': 'manual' if slot['id'] in manual else 'translated'} for slot in context if not calls[slot['id']]]
for slot in never:
    if slot['id'] in (4615, 4616):
        slot.update(classification='unreachable_defeated_master', why=(
            'Activated 137013 listens to its own master being defeated, but status skills require a living master. '
            'The implicit defeat-disposal skill bypasses that guard and removes Activated first. '
            'The custom defeated handler has no bypassDefeatedFilter, so neither its predicate nor action runs.'),
            evidence=['reference/piovium/packages/core/src/gts/vm_impl/skill.ts:263',
                      'reference/piovium/packages/core/src/gts/vm_impl/entity.ts:204',
                      'definitions/skills/skill_model.cpp:130',
                      'definitions/entities/entity_model.cpp:47'])
    else:
        slot.update(classification='not_yet_observed', why='No recorded game has reached this context slot.')
newly_covered = []
if args.baseline:
    baseline = json.loads((ROOT / args.baseline).read_text())
    newly_covered = [{'id': slot['id'], **first_execution[slot['id']]} for slot in baseline['neverExecuted']
                     if slot['id'] in first_execution]
summary = {'total': len(slots), 'translated': len(translated), 'manual': len(manual), 'bound': len(bound),
           'unbound': len(unbound), 'unboundClassifications': dict(collections.Counter(item['classification'] for item in unbound)),
           'currentContext': len(context), 'executedCurrentContext': len(context) - len(never),
           'neverExecutedCurrentContext': len(never), 'executedDistinctSlots': len(calls),
           'unclassifiedNeverExecuted': sum(slot['classification'] == 'not_yet_observed' for slot in never),
           **({'newlyCoveredCurrentContext': len(newly_covered)} if args.baseline else {})}
output = ROOT / args.output
output.write_text(json.dumps({'summary': summary, 'reports': report_counts, 'unbound': unbound,
                              'neverExecuted': never, 'newlyCovered': newly_covered,
                              'slotCounts': dict(sorted(calls.items()))}, indent=2) + '\n')
print(json.dumps(summary))
if any(item['classification'] == 'reachable' for item in unbound):
    raise SystemExit('Reachable unbound slots remain; inspect the inventory.')
if args.require_classified and summary['unclassifiedNeverExecuted']:
    raise SystemExit('Unclassified never-executed context slots remain.')
