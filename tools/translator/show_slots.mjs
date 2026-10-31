// Prints the C++ translation (or the reasons it fails) of chosen context-lambda slots, ignoring
// excluded_slots.json, for debugging the card tail.
// Usage: node --max-old-space-size=6000 tools/translator/show_slots.mjs <slot id>...

import { readFileSync } from "node:fs";
import { join } from "node:path";
import { cardLambdasInOrder, createTypedProgram, DATA_SOURCE_ROOT } from "./typed_program.mjs";
import { LambdaTranslator, NameTable } from "./lambdas/translate_lambda.mjs";

const engineCpp = new URL("../..", import.meta.url).pathname;
const slots = JSON.parse(readFileSync(join(engineCpp, "cards/generated/lambda_slots.json"), "utf8"));
const wanted = new Set(process.argv.slice(2).map(Number));
const files = [...new Set(slots.filter((slot) => wanted.has(slot.id)).map((slot) => slot.file))];
const { program, checker } = createTypedProgram(files.map((file) => join(DATA_SOURCE_ROOT, file)));

for (const file of files) {
  const lambdas = cardLambdasInOrder(program, join(DATA_SOURCE_ROOT, file));
  const contextSlots = slots.filter((slot) => slot.file === file && slot.kind === "function" && slot.source.startsWith("(__gts_fnArg)"));
  contextSlots.forEach((slot, index) => {
    if (!wanted.has(slot.id)) return;
    const translator = new LambdaTranslator(checker, new NameTable());
    let result;
    try {
      result = translator.translateFunction(lambdas[index]);
    } catch (error) {
      translator.unsupported.push(`crash: ${error.message}`);
    }
    console.log(`===== slot ${slot.id} (${file}:${slot.gts.line})`);
    if (translator.unsupported.length) console.log("unsupported:", [...new Set(translator.unsupported)].join("; "));
    else console.log(result.body.join("\n"));
  });
}
