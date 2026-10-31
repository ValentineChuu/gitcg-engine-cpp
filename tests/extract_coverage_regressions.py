"""Retain Node-recorded groups verbatim as small, tracked optional-access/barrier regressions."""
import gzip
import hashlib
import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent
sources = [('coverage_probe_reference.jsonl.gz', {89}), ('coverage_focused_reference.jsonl.gz', {40, 41}),
           ('sweep_reference.jsonl.gz', {1065})]
groups, provenance = [], []
for filename, wanted in sources:
    found, index, group = set(), -1, []
    def keep():
        if index in wanted:
            raw = b''.join(group)
            groups.append(raw)
            found.add(index)
            provenance.append({'source': f'generated/{filename}', 'index': index,
                               'groupSha256': hashlib.sha256(raw).hexdigest()})
    with gzip.open(ROOT / 'generated' / filename, 'rb') as source:
        for line in source:
            if 'create' in json.loads(line):
                keep()
                index += 1
                group = []
            group.append(line)
    keep()
    assert found == wanted, (filename, found, wanted)
output = ROOT / 'fixtures/coverage_regressions.jsonl.gz'
with output.open('wb') as target:
    with gzip.GzipFile(filename='', fileobj=target, mode='wb', mtime=0) as compressed:
        for group in groups:
            compressed.write(group)
output.with_name('coverage_regressions.json').write_text(json.dumps({
    'description': 'Unmodified Node outputs: missing Yelan Breakthrough error; Anemoboxer absorption/counter and overload switch; Kirara optional chain at initial active selection.',
    'groups': provenance, 'requiredSlots': [2234, 5041, 5042, 5045, 5047, 5049],
}, indent=2) + '\n')
print(f'{len(groups)} unmodified groups -> {output}')
