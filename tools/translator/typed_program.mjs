// A type-checked TypeScript program over every GTS file, through the transpiler's Volar output (the code gtsc
// type-checks: card lambdas get their real context types there). Lambdas are found by their GTS source offset.

import { existsSync, readFileSync, readdirSync, statSync } from "node:fs";
import { join } from "node:path";
import ts from "../../reference/piovium/node_modules/.pnpm/typescript@6.0.3/node_modules/typescript/lib/typescript.js";
import { transpile as transpilePlain, transpileForVolar } from "../../reference/piovium/packages/data/node_modules/@gi-tcg/gts-transpiler/dist/index.js";

export { ts };

export const DATA_SOURCE_ROOT = new URL("../../reference/piovium/packages/data/src", import.meta.url).pathname;
const TRANSPILE_OPTIONS = { runtimeImportSource: "@gi-tcg/core/gts/runtime", providerImportSource: "@gi-tcg/core/gts" };
const COMPILER_OPTIONS = {
  target: ts.ScriptTarget.ES2022,
  module: ts.ModuleKind.ES2022,
  moduleResolution: ts.ModuleResolutionKind.Bundler,
  lib: ["lib.esnext.d.ts"],
  noEmit: true,
  allowImportingTsExtensions: true,
  strict: true,
  skipLibCheck: true,
  customConditions: ["development"],
};

export function gtsFiles(directory = DATA_SOURCE_ROOT) {
  return readdirSync(directory).flatMap((name) => {
    const path = join(directory, name);
    return statSync(path).isDirectory() ? gtsFiles(path) : name.endsWith(".gts") ? [path] : [];
  });
}

// Each GTS file is served to TypeScript as "<file>.gts.ts" holding its Volar code.
const virtualCode = new Map();
function volarOf(gtsFile) {
  if (!virtualCode.has(gtsFile)) virtualCode.set(gtsFile, transpileForVolar(readFileSync(gtsFile, "utf8"), gtsFile, TRANSPILE_OPTIONS));
  return virtualCode.get(gtsFile);
}
const gtsOf = (fileName) => (fileName.endsWith(".gts.ts") ? fileName.slice(0, -3) : null);

export function createTypedProgram(files = gtsFiles()) {
  const host = ts.createCompilerHost(COMPILER_OPTIONS);
  const readSourceFile = host.getSourceFile;
  host.getSourceFile = (fileName, language, onError) => {
    const gts = gtsOf(fileName);
    if (gts && existsSync(gts)) return ts.createSourceFile(fileName, volarOf(gts).code, language, true);
    return readSourceFile(fileName, language, onError);
  };
  const fileExists = host.fileExists;
  host.fileExists = (fileName) => (gtsOf(fileName) ? existsSync(gtsOf(fileName)) : fileExists(fileName));
  host.resolveModuleNameLiterals = (literals, containing) =>
    literals.map((literal) => {
      if (literal.text.endsWith(".gts")) {
        const base = "file://" + (gtsOf(containing) ?? containing);
        return { resolvedModule: { resolvedFileName: new URL(literal.text, base).pathname + ".ts", extension: ".ts", isExternalLibraryImport: false } };
      }
      return ts.resolveModuleName(literal.text, containing, COMPILER_OPTIONS, host);
    });
  const program = ts.createProgram(files.map((file) => file + ".ts"), COMPILER_OPTIONS, host);
  return { program, checker: program.getTypeChecker() };
}

// Arrow functions of a GTS file in the typed program, by the GTS source offset they map back to.
export function lambdasBySourceOffset(program, gtsFile) {
  const source = program.getSourceFile(gtsFile + ".ts");
  const { mappings } = volarOf(gtsFile);
  const toSource = (generatedOffset) => {
    for (const mapping of mappings) {
      for (let index = 0; index < mapping.generatedOffsets.length; index++) {
        const start = mapping.generatedOffsets[index];
        const length = (mapping.generatedLengths ?? mapping.lengths)[index];
        if (generatedOffset >= start && generatedOffset < start + length) return mapping.sourceOffsets[index] + (generatedOffset - start);
      }
    }
    return null;
  };
  const result = new Map();
  const visit = (node) => {
    if (ts.isArrowFunction(node) || ts.isFunctionExpression(node)) {
      const sourceOffset = toSource(node.getStart(source));
      if (sourceOffset !== null && !result.has(sourceOffset)) result.set(sourceOffset, node);
    }
    ts.forEachChild(node, visit);
  };
  visit(source);
  return result;
}

// Every arrow function with the GTS source offset of its body (shorthand lambdas like `when :( ... )` and
// action blocks only map their body back to the source), sorted by that offset.
export function lambdasByBodyOffset(program, gtsFile) {
  const source = program.getSourceFile(gtsFile + ".ts");
  const { mappings } = volarOf(gtsFile);
  const toSource = (generatedOffset) => {
    for (const mapping of mappings) {
      for (let index = 0; index < mapping.generatedOffsets.length; index++) {
        const start = mapping.generatedOffsets[index];
        const length = (mapping.generatedLengths ?? mapping.lengths)[index];
        if (generatedOffset >= start && generatedOffset < start + length) return mapping.sourceOffsets[index] + (generatedOffset - start);
      }
    }
    return null;
  };
  const result = [];
  const visit = (node) => {
    if (ts.isArrowFunction(node)) {
      let body = ts.isBlock(node.body) ? node.body.statements[0] ?? node.body : node.body;
      const bodyOffset = toSource(body.getStart(source));
      if (bodyOffset !== null) result.push({ offset: bodyOffset, node });
    }
    ts.forEachChild(node, visit);
  };
  visit(source);
  return result.sort((first, second) => first.offset - second.offset);
}

// Card lambdas (first parameter __gts_fnArg) of a GTS file in the typed program, in source order, outermost and
// nested alike: the same order the attribute layer numbers its function slots in.
export function cardLambdasInOrder(program, gtsFile) {
  const source = program.getSourceFile(gtsFile + ".ts");
  const result = [];
  const visit = (node) => {
    if (ts.isArrowFunction(node) && node.parameters[0]?.name.getText() === "__gts_fnArg") result.push(node);
    ts.forEachChild(node, visit);
  };
  visit(source);
  return result;
}

// Identifiers and literals of a piece of code, for checking that two transpilations show the same lambda.
export function codeFingerprint(text) {
  return (text.match(/[A-Za-z_$][A-Za-z0-9_$]*|\d+/g) ?? []).filter((word) => word !== "as" && !/^[A-Z][A-Za-z]*Handle$/.test(word)).join(" ");
}

// Definition handles: GTS file -> (exported name -> id), from `id N as Name` / `idHint N as Name` lines.
const handleIdsByFile = new Map();
export function handleIdsOf(gtsFile) {
  if (!handleIdsByFile.has(gtsFile)) {
    const ids = new Map();
    for (const match of readFileSync(gtsFile, "utf8").matchAll(/\bid(?:Hint)?\s+(-?\d+(?:\.\d+)?)\s+as\s+(?:(?:private|public)\s+)?([A-Za-z_$][A-Za-z0-9_$]*)/g)) {
      ids.set(match[2], Number(match[1]));
    }
    handleIdsByFile.set(gtsFile, ids);
  }
  return handleIdsByFile.get(gtsFile);
}

// The id an identifier stands for when it is a definition handle (followed through imports), else null.
export function handleIdOfIdentifier(checker, identifier) {
  let symbol = checker.getSymbolAtLocation(identifier);
  if (!symbol) return null;
  if (symbol.flags & ts.SymbolFlags.Alias) symbol = checker.getAliasedSymbol(symbol);
  for (const declaration of symbol.declarations ?? []) {
    const fileName = declaration.getSourceFile().fileName;
    if (!fileName.endsWith(".gts.ts")) continue;
    const id = handleIdsOf(fileName.slice(0, -3)).get(symbol.getName());
    if (id !== undefined) return id;
  }
  return null;
}

// Handles visible in a GTS file: its own definitions plus the ones it imports (followed by import name).
export function fileScopeHandleIds(gtsFile) {
  const scope = new Map(handleIdsOf(gtsFile));
  const text = readFileSync(gtsFile, "utf8");
  for (const match of text.matchAll(/import\s*\{([^}]*)\}\s*from\s*["']([^"']+\.gts)["']/g)) {
    const imported = handleIdsOf(new URL(match[2], "file://" + gtsFile).pathname);
    for (const part of match[1].split(",")) {
      const [original, local] = part.trim().replace(/^type\s+/, "").split(/\s+as\s+/);
      if (original && imported.has(original)) scope.set(local ?? original, imported.get(original));
    }
  }
  return scope;
}

// Custom events are compared by identity; the definition VM keys them "<file relative to data/src>:<offset of the
// customEvent(...) call in the plain transpile>" (tools/translator/translate.mjs). Same key from a declaration name.
const plainSources = new Map();
export function customEventIdentity(gtsFile, name) {
  if (!plainSources.has(gtsFile)) {
    const code = transpilePlain(readFileSync(gtsFile, "utf8"), gtsFile, TRANSPILE_OPTIONS).code;
    plainSources.set(gtsFile, ts.createSourceFile(gtsFile + ".plain.ts", code, ts.ScriptTarget.ES2022, true));
  }
  let identity = null;
  const visit = (node) => {
    if (ts.isVariableDeclaration(node) && ts.isIdentifier(node.name) && node.name.text === name && node.initializer &&
        ts.isCallExpression(node.initializer) && node.initializer.expression.getText().endsWith("customEvent")) {
      identity = `${gtsFile.slice(DATA_SOURCE_ROOT.length + 1)}:${node.initializer.getStart()}`;
    }
    if (!identity) ts.forEachChild(node, visit);
  };
  visit(plainSources.get(gtsFile));
  return identity;
}

// The custom event identity an identifier stands for (followed through imports), else null.
export function customEventOfIdentifier(checker, identifier) {
  let symbol = checker.getSymbolAtLocation(identifier);
  if (!symbol) return null;
  if (symbol.flags & ts.SymbolFlags.Alias) symbol = checker.getAliasedSymbol(symbol);
  for (const declaration of symbol.declarations ?? []) {
    const fileName = declaration.getSourceFile().fileName;
    if (!fileName.endsWith(".gts.ts")) continue;
    const identity = customEventIdentity(fileName.slice(0, -3), symbol.getName());
    if (identity) return identity;
  }
  return null;
}
