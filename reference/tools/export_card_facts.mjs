// Card facts for the line rules (bridge/turn_line_rules.mjs), read from the GTS card code (Piovium plus
// reference/custom_data): `food;`, `:heal(`, `:drawCards(`, random effects (`:random(`, `:randomSubset(`, random-element
// dice) and dice-giving (`:generateDice(`). Written to the engine's data/card_line_facts.json
// (kept in the repository, so the engines need no Piovium sources to use the rules).
//   node tools/export_card_facts.mjs
import { globSync, readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const engine = join(dirname(fileURLToPath(import.meta.url)), "..");
const output = join(engine, "..", "data", "card_line_facts.json");
const facts = {};
for (const pattern of ["piovium/packages/data/src/**/*.gts", "custom_data/**/*.gts"]) {
  for (const relative of globSync(pattern, { cwd: engine })) {
    if (relative.includes("old_versions")) continue;
    const text = readFileSync(join(engine, relative), "utf8");
    for (const match of text.matchAll(/define card \{([\s\S]*?)\n\};/g)) {
      const body = match[1];
      const id = body.match(/\bid (\d+)/);
      if (!id) continue;
      facts[id[1]] = {
        food: /^\s*food;/m.test(body),
        heals: body.includes(":heal("),
        draws: body.includes(":drawCards("),
        random: /:random\(|:randomSubset\(|generateDice\("randomElement"/.test(body),
        givesDice: body.includes(":generateDice("),
      };
    }
  }
}
writeFileSync(output, JSON.stringify(facts, null, 1));
const count = (key) => Object.values(facts).filter((card) => card[key]).length;
console.log(`${Object.keys(facts).length} cards: ${count("food")} food, ${count("draws")} draw, ${count("random")} random, ${count("givesDice")} dice-giving -> ${output}`);
