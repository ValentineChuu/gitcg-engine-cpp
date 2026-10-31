// Survey of what card lambdas use: every property access and call inside a lambda, keyed by the receiver's
// type (class or alias name) and member name, with counts. This is the API the C++ translation must map.
// Usage: node tools/translator/survey_lambda_api.mjs [limit]

import { createTypedProgram, gtsFiles, ts } from "./typed_program.mjs";

const limit = Number(process.argv[2] ?? 400);
const started = Date.now();
const files = gtsFiles();
const { program, checker } = createTypedProgram(files);
console.error(`program: ${program.getSourceFiles().length} files in ${((Date.now() - started) / 1000).toFixed(1)} s`);

function typeName(type) {
  const symbol = type.aliasSymbol ?? type.getSymbol();
  if (symbol) return symbol.getName();
  return checker.typeToString(type).slice(0, 40);
}

const counts = new Map();
const syntaxCounts = new Map();
for (const file of files) {
  const source = program.getSourceFile(file + ".ts");
  const insideLambda = (node) => {
    for (let parent = node.parent; parent; parent = parent.parent) {
      if (ts.isArrowFunction(parent) && parent.parameters.some((parameter) => parameter.name.getText() === "__gts_fnArg")) return true;
    }
    return false;
  };
  const visit = (node) => {
    if (insideLambda(node)) {
      syntaxCounts.set(ts.SyntaxKind[node.kind], (syntaxCounts.get(ts.SyntaxKind[node.kind]) ?? 0) + 1);
      if (ts.isPropertyAccessExpression(node)) {
        const receiver = typeName(checker.getTypeAtLocation(node.expression));
        const key = `${receiver}.${node.name.text}`;
        counts.set(key, (counts.get(key) ?? 0) + 1);
      }
    }
    ts.forEachChild(node, visit);
  };
  visit(source);
}
console.log("== members (receiver type . member) ==");
for (const [key, count] of [...counts].sort((a, b) => b[1] - a[1]).slice(0, limit)) console.log(String(count).padStart(6), key);
console.log(`(${counts.size} distinct)`);
console.log("== syntax kinds inside lambdas ==");
for (const [kind, count] of [...syntaxCounts].sort((a, b) => b[1] - a[1])) console.log(String(count).padStart(7), kind);
