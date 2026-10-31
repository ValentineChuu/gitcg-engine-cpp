// The equivalent Node operation: retain an immutable game root in batches, with no redeal or rule execution.
const count = Number(process.argv[2] ?? 100000000);
const state = Object.freeze({ currentTurn: 0, players: Object.freeze([{}, {}]) });
const copies = [];
let checked = 0;
const started = performance.now();
for (let i = 0; i < count; ++i) {
  copies.push(state);
  if (copies.length === 10000) { for (const copy of copies) checked += copy.currentTurn + 1; copies.length = 0; }
}
for (const copy of copies) checked += copy.currentTurn + 1;
copies.length = 0;
const seconds = (performance.now() - started) / 1000;
console.log(JSON.stringify({ backend: 'node', copies: count, checked, seconds, copiesPerSecond: count / seconds }));
if (checked !== count) process.exitCode = 1;
