// Builds the translated card code with `make -k`, maps every compile error back to the slot whose binding holds
// that line, adds those slots to cards/translated/excluded_slots.json (with the error), re-emits, and repeats
// until the build is clean. Excluded slots stay unbound and are listed for new translator rules or hand ports.
// Usage: node tools/translator/exclude_failing_slots.mjs [maximum rounds]

import { execFileSync, spawnSync } from "node:child_process";
import { existsSync, readFileSync, writeFileSync } from "node:fs";
import { join } from "node:path";

const engineCpp = new URL("../..", import.meta.url).pathname;
const excludedPath = join(engineCpp, "cards/translated/excluded_slots.json");
const maximumRounds = Number(process.argv[2] ?? 6);

function slotAtLine(file, line) {
  const lines = readFileSync(file, "utf8").split("\n");
  for (let index = Math.min(line, lines.length) - 1; index >= 0; index--) {
    const match = lines[index].match(/slots\.bind\((\d+),/);
    if (match) return Number(match[1]);
  }
  return null;
}

for (let round = 1; round <= maximumRounds; round++) {
  execFileSync("node", ["--max-old-space-size=6000", join(engineCpp, "tools/translator/emit_bindings.mjs")], { stdio: "inherit" });
  execFileSync("free", ["-g"], { stdio: "inherit" });
  const build = spawnSync("make", ["-C", engineCpp, "-k", "-j6"], { encoding: "utf8", maxBuffer: 1 << 28 });
  const errors = [...(build.stdout + build.stderr).matchAll(/^(cards\/translated\/[^:]+\.cpp):(\d+):\d+: error: (.*)$/gm)];
  if (errors.length === 0 && build.status === 0) {
    console.log(`round ${round}: clean build`);
    process.exit(0);
  }
  const excluded = existsSync(excludedPath) ? JSON.parse(readFileSync(excludedPath, "utf8")) : {};
  let added = 0;
  for (const [, file, line, message] of errors) {
    const slot = slotAtLine(join(engineCpp, file), Number(line));
    if (slot !== null && !excluded[slot]) {
      excluded[slot] = message;
      added++;
    }
  }
  writeFileSync(excludedPath, JSON.stringify(excluded, null, 1));
  console.log(`round ${round}: ${errors.length} errors, ${added} slots newly excluded (${Object.keys(excluded).length} in total)`);
  if (added === 0) {
    console.log("errors outside slot bindings remain:");
    console.log((build.stdout + build.stderr).split("\n").filter((text) => text.includes("error")).slice(0, 20).join("\n"));
    process.exit(1);
  }
}
