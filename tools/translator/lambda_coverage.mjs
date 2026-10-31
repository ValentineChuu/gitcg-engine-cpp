// Runs the lambda translator over every card lambda and reports how many translate completely, and the most
// common constructs still missing a rule. Usage: node tools/translator/lambda_coverage.mjs [--show N]

import { createTypedProgram, gtsFiles, ts } from "./typed_program.mjs";
import { LambdaTranslator, NameTable } from "./lambdas/translate_lambda.mjs";

const showIndex = process.argv.indexOf("--show");
const show = showIndex >= 0 ? Number(process.argv[showIndex + 1]) : 0;
const files = gtsFiles();
const { program, checker } = createTypedProgram(files);

let total = 0;
let complete = 0;
const reasons = new Map();
const samples = [];
for (const file of files) {
  const source = program.getSourceFile(file + ".ts");
  const visit = (node) => {
    if (ts.isArrowFunction(node) && node.parameters[0]?.name.getText() === "__gts_fnArg") {
      total++;
      const translator = new LambdaTranslator(checker, new NameTable());
      let result;
      try {
        result = translator.translateFunction(node);
      } catch (error) {
        translator.unsupported.push(`crash: ${error.message.slice(0, 60)}`);
      }
      if (translator.unsupported.length === 0) {
        complete++;
        if (samples.length < show) samples.push({ text: node.getText().slice(0, 300), cpp: result.body.join("\n") });
      }
      for (const reason of new Set(translator.unsupported)) reasons.set(reason, (reasons.get(reason) ?? 0) + 1);
      return;
    }
    ts.forEachChild(node, visit);
  };
  visit(source);
}
console.log(`${complete}/${total} card lambdas translate completely (${((100 * complete) / total).toFixed(1)}%)`);
console.log("most common missing rules (lambdas affected):");
for (const [reason, count] of [...reasons].sort((a, b) => b[1] - a[1]).slice(0, 60)) console.log(String(count).padStart(6), reason);
for (const sample of samples) console.log("\n--- TS:\n" + sample.text + "\n--- C++:\n" + sample.cpp);
