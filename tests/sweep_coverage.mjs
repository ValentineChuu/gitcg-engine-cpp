// Coverage only through messages that the C++ worker matched. Untranslated or different continuations cannot
// count as C++ coverage. Presence and behavioral activity are reported separately.
import { existsSync, readFileSync, writeFileSync } from 'node:fs';
const directory = new URL('generated/', import.meta.url);
const node = JSON.parse(readFileSync(new URL('sweep_node_coverage.json', directory)));
const sources = [{ node, replay: JSON.parse(readFileSync(new URL('sweep_replay.json', directory))) }];
if (existsSync(new URL('sweep_opening_coverage.json', directory))) sources.push({
  node: JSON.parse(readFileSync(new URL('sweep_opening_coverage.json', directory))),
  replay: JSON.parse(readFileSync(new URL('sweep_opening_replay.json', directory))),
});
const seen = new Set(), acted = new Set(), nodeSeen = new Set(), nodeActed = new Set();
for (const { node: cohort, replay } of sources) for (const game of cohort.games) {
  const result = replay.results.find(result => result.index === game.game);
  if (!result) continue;
  for (const entry of game.seenAt) {
    nodeSeen.add(entry.id);
    if (entry.decision <= result.decisions) seen.add(entry.id);
  }
  for (const entry of [...game.triggeredAt, ...game.chosenAt]) {
    nodeActed.add(entry.id);
    if (entry.afterDecision < result.decisions) acted.add(entry.id);
  }
}
const neverPresent = node.definitions.filter(d => !seen.has(d.id));
const neverActed = node.definitions.filter(d => !acted.has(d.id));
const groups = {};
for (const definition of node.definitions) {
  const count = groups[definition.type] ??= { total: 0, nodePresent: 0, nodeActed: 0, cppPresent: 0, cppActed: 0 };
  count.total++; count.nodePresent += nodeSeen.has(definition.id); count.nodeActed += nodeActed.has(definition.id);
  count.cppPresent += seen.has(definition.id); count.cppActed += acted.has(definition.id);
}
const result = { groups, neverPresent, neverActed };
writeFileSync(new URL('sweep_coverage.json', directory), JSON.stringify(result, null, 2) + '\n');
console.log(JSON.stringify({ groups, neverPresent: neverPresent.length, neverActed: neverActed.length }));
