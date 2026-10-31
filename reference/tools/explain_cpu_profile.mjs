// Self time per function in a Node .cpuprofile, with minified engine-bundle frames mapped back to their source
// through the bundle's source map (for profiling the engine).
//   node tools/explain_cpu_profile.mjs <file.cpuprofile> [top] [name to show the callers of, e.g. "n @ src/io.ts:160"]
import { readFileSync } from "node:fs";
import { SourceMap } from "node:module";
import { fileURLToPath } from "node:url";

const [file, topArgument = "30", callersOf] = process.argv.slice(2);
const profile = JSON.parse(readFileSync(file, "utf8"));
const nodes = new Map(profile.nodes.map((node) => [node.id, node]));
const maps = new Map();
function sourceMap(url) {
  if (!url.startsWith("file://") || !url.endsWith(".js")) return null;
  if (!maps.has(url)) {
    try { maps.set(url, new SourceMap(JSON.parse(readFileSync(fileURLToPath(url) + ".map", "utf8")))); } catch { maps.set(url, null); }
  }
  return maps.get(url);
}
const interval = (profile.endTime - profile.startTime) / profile.samples.length / 1000;
function nameOf(frame) {
  let name = `${frame.functionName || "(anonymous)"} @ ${frame.url.split("/").at(-1) || "(native)"}`;
  const map = sourceMap(frame.url);
  if (map) {
    const entry = map.findEntry(frame.lineNumber, frame.columnNumber);
    if (entry?.originalSource) name = `${entry.name ?? frame.functionName} @ ${entry.originalSource.split("/").slice(-2).join("/")}:${entry.originalLine + 1}`;
  }
  return name;
}
const parents = new Map();
for (const node of profile.nodes) for (const child of node.children ?? []) parents.set(child, node.id);
const selfTime = new Map();
const callers = new Map();
let inclusive = 0;
for (const sample of profile.samples) {
  const name = nameOf(nodes.get(sample).callFrame);
  selfTime.set(name, (selfTime.get(name) ?? 0) + interval);
  if (callersOf) {  // time with the function anywhere on the stack (inclusive)
    for (let id = sample; id !== undefined; id = parents.get(id)) {
      if (nameOf(nodes.get(id).callFrame) === callersOf) { inclusive += interval; break; }
    }
  }
  if (callersOf && name === callersOf) {
    const chain = [];
    for (let id = parents.get(sample); id !== undefined && chain.length < 4; id = parents.get(id)) chain.push(nameOf(nodes.get(id).callFrame));
    const key = chain.join("  <-  ");
    callers.set(key, (callers.get(key) ?? 0) + interval);
  }
}
const total = [...selfTime.values()].reduce((sum, ms) => sum + ms, 0);
console.log(`total ${(total / 1000).toFixed(1)} s`);
for (const [name, ms] of [...selfTime].sort((a, b) => b[1] - a[1]).slice(0, Number(topArgument))) {
  console.log(`${(100 * ms / total).toFixed(1).padStart(5)}%  ${name}`);
}
if (callersOf) {
  console.log(`\n${callersOf}: ${(100 * inclusive / total).toFixed(1)}% including what it calls; callers of its own time:`);
  for (const [chain, ms] of [...callers].sort((a, b) => b[1] - a[1]).slice(0, 8)) console.log(`${(100 * ms / total).toFixed(1).padStart(5)}%  ${chain}`);
}
