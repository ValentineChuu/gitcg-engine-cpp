// Binds translated card code to the definition VM's lambda slots (cards/generated/lambda_slots.json): one C++ file
// per GTS file under cards/translated/, plus bind_all.cpp. A slot that doesn't translate stays unbound (calling it
// throws "lambda slot N not translated"); the report lists them.
// Usage: node tools/translator/emit_bindings.mjs

import { existsSync, mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { cardLambdasInOrder, createTypedProgram, DATA_SOURCE_ROOT, fileScopeHandleIds, ts } from "./typed_program.mjs";
import { LambdaTranslator, NameTable } from "./lambdas/translate_lambda.mjs";
import { translateQueryText } from "./lambdas/translate_query.mjs";

// Preserve timestamps for unchanged outputs so a translator retry rebuilds only affected files.
function writeIfChanged(path, contents) {
  if (!existsSync(path) || readFileSync(path, "utf8") !== contents) writeFileSync(path, contents);
}

const engineCpp = new URL("../..", import.meta.url).pathname;
const slots = JSON.parse(readFileSync(join(engineCpp, "cards/generated/lambda_slots.json"), "utf8"));
const outputRoot = join(engineCpp, "cards/translated");
// Slots whose translation doesn't compile (exclude_failing_slots.mjs): left unbound.
const excludedPath = join(outputRoot, "excluded_slots.json");
const excluded = existsSync(excludedPath) ? JSON.parse(readFileSync(excludedPath, "utf8")) : {};

const slotsByFile = new Map();
for (const slot of slots) {
  if (!slotsByFile.has(slot.file)) slotsByFile.set(slot.file, []);
  slotsByFile.get(slot.file).push(slot);
}
const gtsPaths = [...slotsByFile.keys()].map((file) => join(DATA_SOURCE_ROOT, file));
const { program, checker } = createTypedProgram(gtsPaths);

function functionName(file) {
  return "bindTranslated_" + file.replace(/\.gts$/, "").replace(/[^A-Za-z0-9]/g, "_");
}

// Lambdas without a skill context that follow a fixed pattern (description and hint getters):
// they get the state, the entity id and the extension state from the VM.
function patternBody(source, names) {
  const text = source.replace(/\s+/g, " ").trim();
  let match = text.match(/^\((?:\w+|_|\{[^}]*\}), (\w+)(?:, \w+)?\) => \1\.variables\.(\w+)$/);
  if (match) return [`return JsonValue(getEntityById(*call.state, call.selfId).variables().valueOr(${names.use(match[2])}, 0));`];
  match = text.match(/^\((\w+)\) => \1\.roundNumber$/);
  if (match) return ["return JsonValue(call.state->roundNumber);"];
  match = text.match(/^\((?:\w+|_), \{ area \}, (\w+)\) => \1\.(\w+)\[(flip\()?area\.who\)?\](\.length)?$/);
  if (match) {
    const who = match[3] ? "1 - getEntityArea(*call.state, call.selfId).who" : "getEntityArea(*call.state, call.selfId).who";
    const entry = `(*call.extensionState)[${JSON.stringify(match[2])}].asArray().at(${who})`;
    return [`return JsonValue(${match[4] ? `static_cast<double>(${entry}.asArray().size())` : `${entry}.asNumber()`});`];
  }
  return null;
}

const untranslated = [];
let bound = 0;
const functionNames = [];
for (const [file, fileSlots] of slotsByFile) {
  const gtsPath = join(DATA_SOURCE_ROOT, file);
  // Card lambdas pair up with the file's context-lambda slots by source order (checked: equal counts per file).
  const lambdas = cardLambdasInOrder(program, gtsPath);
  const contextSlots = fileSlots.filter((slot) => slot.kind === "function" && slot.source.startsWith("(__gts_fnArg)"));
  if (lambdas.length !== contextSlots.length) throw new Error(`${file}: ${lambdas.length} typed lambdas vs ${contextSlots.length} slots`);
  const lambdaOfSlot = new Map(contextSlots.map((slot, index) => [slot.id, lambdas[index]]));
  const names = new NameTable();
  const bindings = [];
  for (const slot of fileSlots) {
    let body = null;
    let reasons = [];
    if (slot.kind === "query") {
      const query = translateQueryText(slot.source, fileScopeHandleIds(gtsPath));
      if (query.code) body = [`return js::ids(c.queryAll(${query.code}));`];
      else reasons = query.unsupported;
    } else {
      const node = lambdaOfSlot.get(slot.id);
      if (!node) {
        body = patternBody(slot.source, names);
        if (!body) reasons = [`parameters ${slot.source.slice(0, slot.source.indexOf("=>")).trim()}`];
      } else {
        const translator = new LambdaTranslator(checker, names);
        let result;
        try {
          result = translator.translateFunction(node);
        } catch (error) {
          translator.unsupported.push(`crash: ${error.message}`);
        }
        if (translator.unsupported.length === 0) {
          body = result.returnsValue ? result.body.map((line) => line.replace(/^(\s*)return (.*);$/, "$1return js::toJson($2);")) : [...result.body.map((line) => line.replace(/^(\s*)return;$/, "$1return JsonValue();").replace(/^(\s*)return (.+);$/, "$1return js::toJson($2);")), "return JsonValue();"];
        } else {
          reasons = translator.unsupported;
        }
      }
    }
    if (body && excluded[slot.id]) {
      reasons = [`compile: ${excluded[slot.id]}`];
      body = null;
    }
    if (body) {
      bound++;
      bindings.push(`  // ${slot.file}:${slot.gts.line}\n  slots.bind(${slot.id}, [](LambdaInvocation& call) -> JsonValue {\n${body[0].includes("call.state") ? "" : "    SkillContext& c = *call.context;\n"}${body.map((line) => "    " + line).join("\n")}\n  });`);
    } else {
      untranslated.push({ id: slot.id, file: slot.file, line: slot.gts.line, kind: slot.kind, reasons: [...new Set(reasons)] });
    }
  }
  const name = functionName(file);
  functionNames.push(name);
  const output = join(outputRoot, file + ".cpp");
  mkdirSync(dirname(output), { recursive: true });
  writeIfChanged(output, `// Generated by tools/translator/emit_bindings.mjs from ${file}; do not edit.
#include "cards/translated/translated_card_support.h"

namespace gitcg {

namespace {
${names.declarations().join("\n")}
}  // namespace

void ${name}(LambdaSlots& slots) {
${bindings.join("\n")}
}

}  // namespace gitcg
`);
}
writeIfChanged(join(outputRoot, "bind_all.cpp"), `// Generated by tools/translator/emit_bindings.mjs; do not edit.
#include "definitions/attributes/lambda_slots.h"

namespace gitcg {

${functionNames.map((name) => `void ${name}(LambdaSlots& slots);`).join("\n")}

void bindTranslatedCardCode(LambdaSlots& slots) {
${functionNames.map((name) => `  ${name}(slots);`).join("\n")}
}

}  // namespace gitcg
`);
writeIfChanged(join(outputRoot, "untranslated.json"), JSON.stringify(untranslated, null, 1));
console.log(`bound ${bound} of ${slots.length} slots; ${untranslated.length} untranslated (cards/translated/untranslated.json)`);
