// Translates a `$` query expression from its text (query slots carry no lambda): a chain of getters and calls on
// `$` becomes the same chain on the C++ builder; arguments are numbers, strings, enum members or nested queries.

import { ts } from "../typed_program.mjs";

const ENUM_OBJECTS = { DiceType: "dice", DamageType: "damage", Reaction: "reaction", Aura: "aura" };
const QUERY_NAMES = { union: "union_", not: "not_" };

export function translateQueryText(text, handleIds = new Map()) {
  const unsupported = [];
  const source = ts.createSourceFile("query.ts", text, ts.ScriptTarget.ES2022, true);
  const statement = source.statements[0];
  const translate = (node) => {
    if (ts.isParenthesizedExpression(node)) return translate(node.expression);
    if (ts.isIdentifier(node)) {
      if (node.text === "$") return "query::dollar()";
      if (handleIds.has(node.text)) return `${handleIds.get(node.text)} /* ${node.text} */`;
      unsupported.push(`query identifier ${node.text}`);
      return node.text;
    }
    if (ts.isNumericLiteral(node)) return node.text;
    if (ts.isStringLiteral(node)) return JSON.stringify(node.text);
    if (ts.isPropertyAccessExpression(node)) {
      if (ts.isIdentifier(node.expression) && ENUM_OBJECTS[node.expression.text]) return `${ENUM_OBJECTS[node.expression.text]}::${node.name.text}`;
      const key = ts.isPropertyAccessExpression(node.expression) && node.expression.name.text === "keys";
      return `${translate(node.expression)}.${QUERY_NAMES[node.name.text] ?? node.name.text}${key ? "" : "()"}`;
    }
    if (ts.isCallExpression(node) && ts.isPropertyAccessExpression(node.expression)) {
      const receiver = translate(node.expression.expression);
      return `${receiver}.${QUERY_NAMES[node.expression.name.text] ?? node.expression.name.text}(${node.arguments.map(translate).join(", ")})`;
    }
    unsupported.push(`query ${ts.SyntaxKind[node.kind]}`);
    return "";
  };
  if (!statement || !ts.isExpressionStatement(statement)) return { code: null, unsupported: ["query text"] };
  const code = translate(statement.expression);
  return unsupported.length ? { code: null, unsupported } : { code, unsupported };
}
