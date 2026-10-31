// Survey: which expression kinds appear as attribute positionals in the transpiled GTS card data, so the
// translator's constant evaluation covers them. Prints counts per syntax kind with an example each.
// Usage: node tools/translator/survey_positionals.mjs

import { readdirSync, readFileSync, statSync } from "node:fs";
import { join } from "node:path";
import ts from "../../reference/piovium/node_modules/.pnpm/typescript@6.0.3/node_modules/typescript/lib/typescript.js";
import { transpile } from "../../reference/piovium/packages/data/node_modules/@gi-tcg/gts-transpiler/dist/index.js";

const sourceRoot = new URL("../../reference/piovium/packages/data/src", import.meta.url).pathname;
const options = { runtimeImportSource: "@gi-tcg/core/gts/runtime", providerImportSource: "@gi-tcg/core/gts" };

function gtsFiles(directory) {
  return readdirSync(directory).flatMap((name) => {
    const path = join(directory, name);
    return statSync(path).isDirectory() ? gtsFiles(path) : name.endsWith(".gts") ? [path] : [];
  });
}

const counts = new Map();
function record(kind, example) {
  const entry = counts.get(kind) ?? { count: 0, example };
  entry.count++;
  counts.set(kind, entry);
}

for (const file of gtsFiles(sourceRoot)) {
  const code = transpile(readFileSync(file, "utf8"), file, options).code;
  const source = ts.createSourceFile(file + ".ts", code, ts.ScriptTarget.ES2022, true);
  const visit = (node) => {
    // positionals: () => [ ... ]
    if (ts.isPropertyAssignment(node) && node.name.getText() === "positionals" && ts.isArrowFunction(node.initializer)) {
      const body = node.initializer.body;
      if (ts.isArrayLiteralExpression(body)) {
        for (const element of body.elements) {
          const kind = ts.SyntaxKind[element.kind];
          const detail = ts.isPropertyAccessExpression(element) ? `${kind}:${element.expression.getText()}` : kind;
          record(detail, element.getText().slice(0, 80));
        }
      } else {
        record(`non-array:${ts.SyntaxKind[body.kind]}`, body.getText().slice(0, 80));
      }
    }
    ts.forEachChild(node, visit);
  };
  visit(source);
}
for (const [kind, { count, example }] of [...counts].sort((a, b) => b[1].count - a[1].count)) {
  console.log(String(count).padStart(6), kind.padEnd(40), example.replaceAll("\n", " "));
}
