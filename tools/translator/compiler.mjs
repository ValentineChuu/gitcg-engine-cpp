// Local compiler dependencies and the depth-first ES module evaluation graph.
import { readFileSync, existsSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import path from 'node:path';
export const root = path.resolve(fileURLToPath(new URL('../..', import.meta.url)));
export const dataRoot = path.join(root, 'reference/piovium/packages/data/src');
export const outputRoot = path.join(root, 'cards/generated');
export const ts = (await import(path.join(root, 'reference/piovium/node_modules/.pnpm/typescript@6.0.3/node_modules/typescript/lib/typescript.js'))).default;
const { transpile } = await import(path.join(root, 'reference/piovium/packages/data/node_modules/@gi-tcg/gts-transpiler/dist/index.js'));
const options = {runtimeImportSource:'@gi-tcg/core/gts/runtime', providerImportSource:'@gi-tcg/core/gts'};
export function moduleGraph(entry = path.join(dataRoot, 'index.ts')) {
  const modules = new Map(), order = [];
  function visit(file) {
    file = path.resolve(file);
    if (modules.has(file)) return;
    const source = readFileSync(file,'utf8');
    const translated = file.endsWith('.gts') ? transpile(source, file, options) : {code:source};
    const ast = ts.createSourceFile(file, translated.code, ts.ScriptTarget.Latest, true, ts.ScriptKind.TS);
    const module = {file, source, ...translated, ast, imports:new Map(), declarations:new Map(), exports:new Map()};
    modules.set(file,module);
    for (const statement of ast.statements) {
      if ((ts.isImportDeclaration(statement) || ts.isExportDeclaration(statement)) && statement.moduleSpecifier) {
        const name = statement.moduleSpecifier.text;
        if (!name.startsWith('.')) continue;
        const dependency = path.resolve(path.dirname(file), name);
        if (!existsSync(dependency)) throw Error(`Missing import ${name} in ${file}`);
        if (ts.isImportDeclaration(statement) && statement.importClause?.namedBindings && ts.isNamedImports(statement.importClause.namedBindings)) {
          for (const spec of statement.importClause.namedBindings.elements) module.imports.set(spec.name.text, {file:dependency,name:spec.propertyName?.text ?? spec.name.text});
        }
        visit(dependency);
      }
      if (ts.isVariableStatement(statement)) for (const declaration of statement.declarationList.declarations) {
        if (ts.isIdentifier(declaration.name)) {
          module.declarations.set(declaration.name.text,declaration.initializer);
          if (statement.modifiers?.some(m => m.kind === ts.SyntaxKind.ExportKeyword)) module.exports.set(declaration.name.text, declaration.name.text);
        }
      }
      if (ts.isExportDeclaration(statement) && statement.exportClause && ts.isNamedExports(statement.exportClause)) {
        for (const spec of statement.exportClause.elements) {
          const name = spec.propertyName?.text ?? spec.name.text;
          if (statement.moduleSpecifier) module.imports.set(spec.name.text, {file:path.resolve(path.dirname(file),statement.moduleSpecifier.text),name});
          module.exports.set(spec.name.text, statement.moduleSpecifier ? spec.name.text : name);
        }
      }
    }
    order.push(module);
  }
  visit(entry);
  return {modules,order};
}
export const property = (node,name) => node.properties.find(p => p.name && (p.name.text ?? p.name.getText()) === name)?.initializer;
export const unwrap = node => {
  while (ts.isParenthesizedExpression(node) || ts.isAsExpression(node) || ts.isNonNullExpression(node) || ts.isSatisfiesExpression(node)) node=node.expression;
  return node;
};
export function attributeChildren(node) {
  const named = property(node,'named');
  return named && ts.isObjectLiteralExpression(named) ? property(named,'attributes').elements : [];
}
export function positionals(node) { return unwrap(property(node,'positionals').body).elements; }
