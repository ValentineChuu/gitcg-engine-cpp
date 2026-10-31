"""Turn the measured missing-slot inventory into targeted definition/character probes."""
import json
import math
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
inventory = json.loads((ROOT / 'docs/coverage_stage_inventory.json').read_text())
data = json.loads((ROOT / 'tests/generated/definitions_node.json').read_text())
characters = {item['id']: item for item in data['characters']}
entities = {item['id']: item for item in data['entities']}
targets = {}
for slot in inventory['neverExecuted']:
    for owner in slot['owners']:
        identifier = owner['definition']
        if owner['type'] == 'skill':
            candidates = [item['id'] for item in data['characters']
                          if any(math.floor(skill['id']) == identifier for skill in item['skills'])]
            if not candidates:
                raise ValueError(f'No current character for skill {identifier}')
            identifier = candidates[0]
        if identifier in characters or identifier in entities:
            targets.setdefault(identifier, []).append(slot['id'])
probes = []
for identifier, slots in targets.items():
    for variant, opponents in enumerate(([1301, 1401, 1201], [1601, 1701, 1101], [1208, 1212, 1215])):
        probe = {'definition': identifier, 'name': f'missing-{min(slots)}-variant-{variant}',
                 'targets': sorted(set(slots)), 'opponents': opponents, 'turn': variant % 2,
                 'auras': [[2, 2, 2], [1, 1, 1]], 'cards': [333003, 332004]}
        entity = entities.get(identifier)
        if entity and entity['type'] == 'equipment' and variant != 0:
            probe['equipped'] = True
        if 311100 <= identifier < 311600:
            first = {3111: 1303, 3112: 1401, 3113: 1301, 3114: 1404, 3115: 1101}[identifier // 100]
            probe['characters'] = [first, *[item for item in (1101, 1201, 1301) if item != first]][:3]
        if variant == 2:
            probe['health'] = [[12, 12, 12], [10, 10, 10]]
            probe['characterVariables'] = [[{'maxHealth': 12}] * 3, [{}, {}, {}]]
        probes.append(probe)
output = ROOT / 'tests/generated/coverage_probe_cases.json'
output.write_text(json.dumps(probes, indent=2) + '\n')
print(f'{len(targets)} definitions, {len(probes)} probes -> {output}')
