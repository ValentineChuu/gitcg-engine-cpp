// Translates one card lambda (an arrow function taking the skill context) into a C++ lambda body, using the
// checker's types. Constructs without a rule are recorded in `unsupported` instead of guessed.

import { customEventOfIdentifier, handleIdOfIdentifier, ts } from "../typed_program.mjs";
import {
  CHARACTER_PROPERTY_RULES, CHARACTER_RULES, CONTEXT_PROPERTY_RULES, CONTEXT_RULES, ENTITY_PROPERTY_RULES, ENTITY_RULES,
} from "./member_rules.mjs";
import {
  ACTION_INFO_PROPERTY_RULES, ANY_STATE_PROPERTY_RULES, AREA_PROPERTY_RULES, DAMAGE_INFO_PROPERTY_RULES, ANY_STATE_RULES, ARRAY_PROPERTY_RULES, ARRAY_RULES, DEFINITION_PROPERTY_RULES,
  EXTRA_CONTEXT_RULES, PLAYER_PROPERTY_RULES, SKILL_DEFINITION_PROPERTY_RULES, INITIATIVE_SKILL_CONFIG_PROPERTY_RULES, SKILL_INFO_PROPERTY_RULES, SWITCH_INFO_PROPERTY_RULES,
  TAGS_RULES, cppTypeOf,
} from "./more_member_rules.mjs";

const PROPERTY_RULES_BY_KIND = {
  anyState: ANY_STATE_PROPERTY_RULES, definition: DEFINITION_PROPERTY_RULES, player: PLAYER_PROPERTY_RULES,
  skillInfo: SKILL_INFO_PROPERTY_RULES, skillDefinition: SKILL_DEFINITION_PROPERTY_RULES, switchInfo: SWITCH_INFO_PROPERTY_RULES,
  initiativeSkillConfig: INITIATIVE_SKILL_CONFIG_PROPERTY_RULES, actionInfo: ACTION_INFO_PROPERTY_RULES, array: ARRAY_PROPERTY_RULES, area: AREA_PROPERTY_RULES, damageInfo: DAMAGE_INFO_PROPERTY_RULES,
};
const CALL_RULES_BY_KIND = { anyState: ANY_STATE_RULES, tags: TAGS_RULES, array: ARRAY_RULES };
import { valueKind } from "./value_kinds.mjs";

const ENUM_OBJECTS = { DiceType: "dice", DamageType: "damage", Reaction: "reaction", Aura: "aura" };
// C++ keywords renamed in the query builder.
const QUERY_NAMES = { union: "union_", not: "not_" };
// Methods whose string arguments are skill types.
const SKILL_TYPE_METHODS = new Set(["isSkillType", "viaSkillType", "isSkillOrTalentOf"]);
const SKILL_TYPES = { normal: "SkillType::Normal", elemental: "SkillType::Elemental", burst: "SkillType::Burst", technique: "SkillType::Technique" };
// Event argument members that are C++ fields, not methods, per C++ class (with the classes they derive from).
const EVENT_FIELDS = {
  EventArg: [],
  PlayerEventArg: ["who"],
  ModifyRollEventArg: ["who", "fixedDice"],
  ActionEventArg: ["who"],
  ModifyActionEventArg: ["who"],
  SwitchActiveEventArg: ["switchInfo"],
  UseSkillEventArg: ["who", "callerArea", "skill"],
  PlayCardEventArg: ["who", "playCardInfo"],
  DamageOrHealEventArg: ["option", "sourceWho", "targetWho"],
  ModifyDamageEventArg: ["option", "sourceWho", "targetWho"],
  ModifyHealEventArg: ["option", "sourceWho", "targetWho"],
  ZeroHealthEventArg: ["option", "sourceWho", "targetWho"],
  EntityEventArg: ["entity", "area", "who"],
  EnterEventArg: ["entity", "area", "who", "overridden"],
  DisposeEventArg: ["entity", "area", "who", "reason", "from", "via"],
  CharacterEventArg: ["character", "who"],
  ReactionEventArg: ["character", "who", "reactionInfo"],
  ModifyReactionEventArg: ["character", "who", "reactionInfo"],
  TransformDefinitionEventArg: ["entity", "who", "oldDefinition", "newDefinition"],
  HandCardInsertedEventArg: ["who", "card", "reason", "overflowed"],
  GenerateDiceEventArg: ["who", "via", "dice"],
  VariableEventArg: ["entity", "area", "who", "info"],
  BeforeVariableEventArg: ["entity", "area", "who", "info"],
  SelectCardEventArg: ["who", "info"],
  CustomEventEventArg: ["entity", "area", "who", "customEvent", "arg"],
  InitiativeSkillArgument: ["targets"],
};
// Methods with tag arguments (strings become interned names).
const TAG_METHODS = new Set(["hasCardTag", "hasOneOfCardTag"]);
// Event argument members that return a DamageInfo.
const DAMAGE_INFO_MEMBERS = new Set(["damageInfo", "healInfo"]);

const BINARY_OPERATORS = {
  [ts.SyntaxKind.EqualsEqualsEqualsToken]: "==", [ts.SyntaxKind.ExclamationEqualsEqualsToken]: "!=",
  [ts.SyntaxKind.LessThanToken]: "<", [ts.SyntaxKind.LessThanEqualsToken]: "<=", [ts.SyntaxKind.GreaterThanToken]: ">",
  [ts.SyntaxKind.GreaterThanEqualsToken]: ">=", [ts.SyntaxKind.PlusToken]: "+", [ts.SyntaxKind.MinusToken]: "-",
  [ts.SyntaxKind.AsteriskToken]: "*", [ts.SyntaxKind.SlashToken]: "/",
};

// Per-file `static const Name` constants for variable names.
// JS local names that are C++ keywords or the generated code's own names get a trailing underscore.
const RESERVED_LOCALS = new Set(["c", "call", "js", "query", "slots", "state", "char", "int", "long", "short", "double", "float",
  "bool", "auto", "signed", "unsigned", "struct", "union", "template", "typename", "namespace", "operator", "register",
  "explicit", "friend", "mutable", "virtual", "volatile", "inline", "private", "protected", "public", "not", "and", "or"]);
function cppLocal(name) {
  return RESERVED_LOCALS.has(name) ? `${name}_` : name;
}

export class NameTable {
  constructor() { this.names = new Map(); }
  use(text) {
    const identifier = `name_${text.replace(/[^A-Za-z0-9]/g, "_")}`;
    this.names.set(identifier, text);
    return identifier;
  }
  declarations() {
    return [...this.names].map(([identifier, text]) => `static const Name ${identifier} = intern(${JSON.stringify(text)});`);
  }
}

export class LambdaTranslator {
  constructor(checker, names) {
    this.checker = checker;
    this.names = names;
    this.unsupported = [];
    this.contextName = "__gts_fnArg";
    this.declaredKinds = new Map();  // local name -> kind of its C++ variable
    this.receiverOverrides = new Map();  // node -> translated value (the present value inside x?.method())
  }

  fail(node, reason) {
    this.unsupported.push(reason);
    return { code: `/* unsupported: ${reason} */`, kind: { kind: "unknown" } };
  }

  kindOf(node) {
    const kind = valueKind(this.checker, this.checker.getTypeAtLocation(node));
    if (kind.kind === "unknown" && !kind.name) kind.name = this.checker.typeToString(this.checker.getTypeAtLocation(node)).slice(0, 50);
    return kind;
  }

  describe(kind) {
    return kind.kind === "unknown" ? `?${kind.name ?? "?"}` : kind.kind;
  }

  translateFunction(node) {
    const [parameter] = node.parameters;
    this.contextName = parameter?.name.getText() ?? "__gts_fnArg";
    const selfKind = this.selfKind(node);
    this.contextInfo = {
      selfKind,
      selfCode: selfKind.kind === "character" ? "c.selfCharacter()" : selfKind.kind === "entity" ? "c.selfEntity()" : "c.self()",
    };
    const returnsValue = !ts.isBlock(node.body);
    const body = returnsValue ? [`return ${this.condition(node.body)};`] : this.block(node.body);
    return { body, returnsValue };
  }

  selfKind(node) {
    const parameterType = this.checker.getTypeAtLocation(node.parameters[0]);
    const self = parameterType.getProperty("self");
    if (!self) return { kind: "unknown" };
    return valueKind(this.checker, this.checker.getTypeOfSymbolAtLocation(self, node));
  }

  // --- statements ---
  block(node) {
    return node.statements.flatMap((statement) => this.statement(statement));
  }

  statement(node) {
    if (ts.isExpressionStatement(node) && ts.isVoidExpression(node.expression)) return [];
    if (ts.isExpressionStatement(node)) return [`${this.expression(node.expression).code};`];
    if (ts.isVariableStatement(node)) {
      return node.declarationList.declarations.flatMap((declaration) => {
        if (!ts.isIdentifier(declaration.name)) return this.destructuring(declaration);
        if (!declaration.initializer) return this.uninitializedVariable(declaration);
        // `let x = null` assigned later: an optional of the first assigned value's type.
        if (declaration.initializer.kind === ts.SyntaxKind.NullKeyword) return this.uninitializedVariable(declaration, true);
        if (ts.isArrayLiteralExpression(declaration.initializer) && declaration.initializer.elements.length === 0) {
          return this.emptyList(declaration);
        }
        let value = this.expression(declaration.initializer);
        // `const x = array[i]` may be undefined past the end (TS types it as present): keep it optional.
        if (declaration.initializer && ts.isElementAccessExpression(declaration.initializer)) {
          const array = this.expression(declaration.initializer.expression);
          if (array.kind.kind === "array") {
            const index = this.expression(declaration.initializer.argumentExpression).code;
            value = { code: `js::at(${array.code}, ${index})`, kind: { kind: "optional", inner: value.kind } };
          }
        }
        this.declaredKinds.set(declaration.name.text, value.kind);
        return `auto ${cppLocal(declaration.name.text)} = ${value.code};`;
      });
    }
    if (ts.isIfStatement(node)) {
      const lines = [`if (${this.condition(node.expression)}) {`, ...this.indented(node.thenStatement), "}"];
      if (node.elseStatement) lines.push("else {", ...this.indented(node.elseStatement), "}");
      return lines;
    }
    if (ts.isBlock(node)) return ["{", ...this.block(node).map((line) => "  " + line), "}"];
    if (ts.isReturnStatement(node)) return [node.expression ? `return ${this.expression(node.expression).code};` : "return;"];
    if (ts.isWhileStatement(node)) {
      return [`while (${this.condition(node.expression)}) {`, ...this.indented(node.statement), "}"];
    }
    if (ts.isForOfStatement(node)) {
      const declaration = node.initializer.declarations?.[0];
      if (!declaration || !ts.isIdentifier(declaration.name)) return [`${this.fail(node, "for-of pattern").code};`];
      return [`for (auto ${cppLocal(declaration.name.text)} : ${this.expression(node.expression).code}) {`, ...this.indented(node.statement), "}"];
    }
    if (ts.isForStatement(node)) {
      const declarations = node.initializer && ts.isVariableDeclarationList(node.initializer) ? node.initializer.declarations : null;
      if (!declarations || declarations.length !== 1 || !ts.isIdentifier(declarations[0].name) || !declarations[0].initializer ||
          !node.condition || !node.incrementor) {
        return [`${this.fail(node, "for statement shape").code};`];
      }
      const name = declarations[0].name.text;
      const start = this.expression(declarations[0].initializer);
      this.declaredKinds.set(name, start.kind);
      return [`for (auto ${cppLocal(name)} = ${start.code}; ${this.condition(node.condition)}; ${this.expression(node.incrementor).code}) {`,
        ...this.indented(node.statement), "}"];
    }
    if (ts.isSwitchStatement(node)) return this.switchStatement(node);
    if (ts.isBreakStatement(node) && !node.label) return ["break;"];
    return [`${this.fail(node, `statement ${ts.SyntaxKind[node.kind]}`).code};`];
  }

  // switch -> a one-pass loop of ifs, so `break` keeps its meaning; cases sharing a body are or-ed.
  switchStatement(node) {
    const subject = this.expression(node.expression);
    const lines = ["do {", `  const auto switchValue = ${subject.code};`];
    // JS takes `default` only when no case matches, wherever it is written. Fall-through is refused below, so each
    // clause group is independent and the default group's test is "its own cases, or none of them".
    const caseTest = (clause) => `switchValue == ${this.expression(clause.expression).code}`;
    const allCases = node.caseBlock.clauses.filter((clause) => ts.isCaseClause(clause)).map(caseTest);
    const noCase = allCases.length ? `!(${allCases.join(" || ")})` : "true";
    let pending = [];
    let pendingDefault = false;
    for (const clause of node.caseBlock.clauses) {
      if (ts.isCaseClause(clause)) pending.push(caseTest(clause));
      else pendingDefault = true;
      if (clause.statements.length === 0) continue;
      const body = clause.statements.flatMap((statement) => this.statement(statement)).map((line) => "    " + line);
      const test = [...pending, ...(pendingDefault ? [noCase] : [])].join(" || ");
      pendingDefault = false;
      let last = clause.statements.at(-1);
      while (ts.isBlock(last) && last.statements.length) last = last.statements.at(-1);
      const falls = !["BreakStatement", "ReturnStatement", "ThrowStatement"].includes(ts.SyntaxKind[last.kind]);
      if (falls) this.fail(clause, "switch fallthrough");
      lines.push(`  if (${test}) {`, ...body, "  }");
      pending = [];
    }
    lines.push("} while (false);");
    return lines;
  }

  indented(statement) {
    const lines = ts.isBlock(statement) ? this.block(statement) : this.statement(statement);
    return lines.map((line) => "  " + line);
  }

  // A boolean condition with JS truthiness made explicit.
  condition(node) {
    if (ts.isParenthesizedExpression(node)) return `(${this.condition(node.expression)})`;
    if (ts.isBinaryExpression(node) && [ts.SyntaxKind.AmpersandAmpersandToken, ts.SyntaxKind.BarBarToken].includes(node.operatorToken.kind)) {
      const joiner = node.operatorToken.kind === ts.SyntaxKind.AmpersandAmpersandToken ? "&&" : "||";
      return `${this.condition(node.left)} ${joiner} ${this.condition(node.right)}`;
    }
    if (ts.isPrefixUnaryExpression(node) && node.operator === ts.SyntaxKind.ExclamationToken) return `!(${this.condition(node.operand)})`;
    // An optional local tested directly: presence (TS may not know it can be undefined).
    if (ts.isIdentifier(node) && this.declaredKinds.get(node.text)?.kind === "optional") return `${node.text}.has_value()`;
    const value = this.expression(node);
    if (value.presence) return value.presence;
    switch (value.kind.kind) {
      case "boolean": return value.code;
      case "number": return `js::truthy(${value.code})`;
      case "optional": return `js::truthy(${value.code})`;
      default: return `js::truthy(${value.code})`;
    }
  }

  // --- expressions ---
  expression(node) {
    if (this.receiverOverrides.has(node)) return this.receiverOverrides.get(node);
    if (ts.isParenthesizedExpression(node) || ts.isAsExpression(node) || ts.isNonNullExpression(node)) {
      const inner = this.expression(node.expression);
      return ts.isParenthesizedExpression(node) ? { ...inner, code: `(${inner.code})`, continueChain: undefined } : inner;
    }
    if (ts.isNumericLiteral(node)) return { code: `${node.text}.0`.replace(/\.(\d+)\.0$/, ".$1"), kind: { kind: "number" }, literal: Number(node.text) };
    if (ts.isStringLiteral(node)) return { code: JSON.stringify(node.text), kind: { kind: "string" }, literal: node.text };
    if (node.kind === ts.SyntaxKind.TrueKeyword) return { code: "true", kind: { kind: "boolean" } };
    if (node.kind === ts.SyntaxKind.FalseKeyword) return { code: "false", kind: { kind: "boolean" } };
    if (ts.isIdentifier(node)) return this.identifier(node);
    if (ts.isPropertyAccessExpression(node)) return this.propertyAccess(node);
    if (ts.isCallExpression(node)) return this.call(node);
    if (ts.isBinaryExpression(node)) return this.binary(node);
    if (ts.isPrefixUnaryExpression(node)) {
      const operand = this.expression(node.operand);
      if (node.operator === ts.SyntaxKind.ExclamationToken) return { code: `!(${this.condition(node.operand)})`, kind: { kind: "boolean" } };
      if (node.operator === ts.SyntaxKind.MinusToken) return { code: `-${operand.code}`, kind: { kind: "number" } };
      if (node.operator === ts.SyntaxKind.PlusToken) {
        return { code: operand.kind.kind === "boolean" ? `(${operand.code} ? 1.0 : 0.0)` : operand.code, kind: { kind: "number" } };
      }
      return this.fail(node, `prefix ${ts.SyntaxKind[node.operator]}`);
    }
    if ((ts.isPostfixUnaryExpression(node) || ts.isPrefixUnaryExpression(node)) && ts.isIdentifier(node.operand) &&
        [ts.SyntaxKind.PlusPlusToken, ts.SyntaxKind.MinusMinusToken].includes(node.operator)) {
      const operator = node.operator === ts.SyntaxKind.PlusPlusToken ? "++" : "--";
      return { code: ts.isPostfixUnaryExpression(node) ? `${node.operand.text}${operator}` : `${operator}${node.operand.text}`, kind: { kind: "number" } };
    }
    if (ts.isConditionalExpression(node)) {
      const whenTrue = this.expression(node.whenTrue);
      const whenFalse = this.expression(node.whenFalse);
      return { code: `(${this.condition(node.condition)} ? ${whenTrue.code} : ${whenFalse.code})`, kind: whenTrue.kind };
    }
    if (ts.isElementAccessExpression(node)) {
      const receiver = this.expression(node.expression);
      if (receiver.kind.kind === "json") {
        return this.fromJson(node, `${receiver.code}.asArray().at(static_cast<size_t>(${this.expression(node.argumentExpression).code}))`);
      }
      if (receiver.kind.kind !== "array") return this.fail(node, `index into ${this.describe(receiver.kind)}`);
      const elementKind = this.kindOf(node);
      const kind = elementKind.kind === "unknown" ? receiver.kind.element : elementKind;
      const access = `js::index(${receiver.code}, ${this.expression(node.argumentExpression).code})`;
      if (kind.kind === "character" && receiver.kind.element.kind === "anyState") return { code: `${access}.asCharacter()`, kind };
      if (kind.kind === "entity" && receiver.kind.element.kind === "anyState") return { code: `${access}.asEntity()`, kind };
      return { code: access, kind };
    }
    if (ts.isArrayLiteralExpression(node) && node.elements.length === 1 && ts.isSpreadElement(node.elements[0])) {
      const items = this.expression(node.elements[0].expression);
      if (items.kind.kind === "array") return items;
    }
    if (ts.isArrayLiteralExpression(node) && node.elements.some((element) => ts.isSpreadElement(element))) {
      const kind = this.kindOf(node);
      const elementType = kind.kind === "array" ? cppTypeOf(kind.element) : null;
      if (!elementType) return this.fail(node, "spread into an untyped list");
      const parts = node.elements.map((element) =>
        ts.isSpreadElement(element) ? this.expression(element.expression).code : `std::vector<${elementType}>{${this.expression(element).code}}`);
      return { code: `js::concat(std::vector<std::vector<${elementType}>>{${parts.join(", ")}})`, kind };
    }
    if (ts.isArrayLiteralExpression(node)) {
      const kind = this.kindOf(node);
      const elementType = kind.kind === "array" ? cppTypeOf(kind.element) : null;
      if (!elementType) return this.fail(node, `array literal of ${kind.kind === "array" ? this.describe(kind.element) : this.describe(kind)}`);
      const elements = node.elements.map((element) => this.expression(element).code);
      return { code: `std::vector<${elementType}>{${elements.join(", ")}}`, kind };
    }
    if (ts.isArrowFunction(node)) return this.nestedFunction(node);
    if (ts.isNewExpression(node) && node.expression.getText() === "Set" && node.arguments?.length === 1) {
      const items = this.expression(node.arguments[0]);
      return { code: `js::distinct(${items.code})`, kind: { kind: "set", element: items.kind.element } };
    }
    return this.fail(node, `expression ${ts.SyntaxKind[node.kind]}`);
  }

  // A callback inside card code (filter/map/some predicates): a C++ lambda capturing by reference.
  nestedFunction(node) {
    const parameters = node.parameters.map((parameter) => `auto&& ${cppLocal(parameter.name.getText())}`);
    const body = ts.isBlock(node.body) ? this.block(node.body) : [`return ${this.expression(node.body).code};`];
    return { code: `[&](${parameters.join(", ")}) { ${body.join(" ")} }`, kind: { kind: "function" } };
  }

  identifier(node) {
    if (node.text === this.contextName) return { code: "c", kind: { kind: "context" } };
    if (node.text === "$") return { code: "query::dollar()", kind: { kind: "query" } };
    const handleId = handleIdOfIdentifier(this.checker, node);
    if (handleId !== null) return { code: `${handleId} /* ${node.text} */`, kind: { kind: "number" }, literal: handleId };
    const moduleConstant = this.moduleConstant(node);
    if (moduleConstant) return moduleConstant;
    const customEvent = customEventOfIdentifier(this.checker, node);
    if (customEvent) return { code: `js::customEvent(${JSON.stringify(customEvent)})`, kind: { kind: "customEvent" }, literal: customEvent };
    const declared = this.declaredKinds.get(node.text);
    const narrowed = this.kindOf(node);
    // TS narrowed an optional local (after `if (x)`): the C++ variable is still an optional.
    if (declared?.kind === "optional" && narrowed.kind !== "optional") return { code: `${cppLocal(node.text)}.value()`, kind: declared.inner };
    return { code: cppLocal(node.text), kind: declared && declared.kind !== "unknown" ? declared : narrowed };
  }

  // `const [a, b] = array` (absent past the end, so optional) and `const { a, b } = object` (plain names only).
  destructuring(declaration) {
    const pattern = declaration.name;
    if (!declaration.initializer) return [`${this.fail(declaration, "destructuring").code};`];
    const source = this.expression(declaration.initializer);
    const temporary = `destructured${declaration.pos}`;
    const lines = [`auto ${temporary} = ${source.code};`];
    const receiver = { code: temporary, kind: source.kind };
    pattern.elements.forEach((element, index) => {
      if (ts.isOmittedExpression(element)) return;
      if (element.dotDotDotToken || element.initializer || !ts.isIdentifier(element.name) ||
          (ts.isObjectBindingPattern(pattern) && element.propertyName)) {
        lines.push(`${this.fail(element, "destructuring").code};`);
        return;
      }
      let value;
      if (ts.isArrayBindingPattern(pattern)) {
        if (source.kind.kind !== "array") value = this.fail(element, "destructuring");
        else value = { code: `js::at(${temporary}, ${index})`, kind: { kind: "optional", inner: source.kind.element } };
      } else {
        value = this.memberOf(element, receiver, element.name.text);
      }
      this.declaredKinds.set(element.name.text, value.kind);
      lines.push(`auto ${cppLocal(element.name.text)} = ${value.code};`);
    });
    return lines;
  }

  // `const list = []` filled by `list.push(value)`: typed from the first pushed value.
  emptyList(declaration) {
    const symbol = this.checker.getSymbolAtLocation(declaration.name);
    let pushed = null;
    const visit = (node) => {
      if (pushed) return;
      if (ts.isCallExpression(node) && ts.isPropertyAccessExpression(node.expression) && node.expression.name.text === "push" &&
          ts.isIdentifier(node.expression.expression) && this.checker.getSymbolAtLocation(node.expression.expression) === symbol) {
        pushed = node.arguments[0];
        return;
      }
      ts.forEachChild(node, visit);
    };
    visit(declaration.parent.parent.parent);
    const element = pushed ? this.kindOf(pushed) : { kind: "unknown" };
    const type = cppTypeOf(element);
    if (!type) return `${this.fail(declaration, `empty list of ${element.kind}`).code};`;
    this.declaredKinds.set(declaration.name.text, { kind: "array", element });
    return `std::vector<${type}> ${cppLocal(declaration.name.text)};`;
  }

  // `let x;` assigned later: typed from the first value assigned to it (JS leaves it undefined until then).
  uninitializedVariable(declaration, optional = false) {
    const symbol = this.checker.getSymbolAtLocation(declaration.name);
    let assigned = null;
    const visit = (node) => {
      if (assigned) return;
      if (ts.isBinaryExpression(node) && node.operatorToken.kind === ts.SyntaxKind.EqualsToken && ts.isIdentifier(node.left) &&
          this.checker.getSymbolAtLocation(node.left) === symbol) {
        assigned = node.right;
        return;
      }
      ts.forEachChild(node, visit);
    };
    visit(declaration.parent.parent.parent);
    const kind = assigned ? this.kindOf(assigned) : { kind: "unknown" };
    const type = cppTypeOf(kind);
    if (!type) return `${this.fail(declaration, `let without initializer (${kind.kind})`).code};`;
    if (optional) {
      this.declaredKinds.set(declaration.name.text, { kind: "optional", inner: kind });
      return `std::optional<${type}> ${cppLocal(declaration.name.text)};`;
    }
    this.declaredKinds.set(declaration.name.text, kind);
    return `${type} ${cppLocal(declaration.name.text)}{};`;
  }

  // A value that may be absent used as a receiver: `x?.m` keeps it optional, plain `x.m` (TS narrowed it) unwraps.
  unwrapOptional(node, receiver) {
    if (receiver.kind.kind !== "optional") return receiver;
    // A TS non-null assertion is not a runtime check. Empty handles must fail like JS, not invoke C++ UB.
    return { code: `(${receiver.code}).value()`, kind: receiver.kind.inner };
  }

  // Keep the unflattened chain body for later segments. x?.a.b skips b only when x is absent;
  // if x exists but a is absent, plain .b must still throw. Flatten only the final expression's value.
  optionalChain(receiver, inner) {
    const operation = inner.kind.kind === "void" ? "ifPresent"
      : inner.kind.kind === "optional" ? "flatMapOptional" : "mapOptional";
    return {
      code: `js::${operation}(${receiver.code}, [&](const auto& value) { ${inner.kind.kind === "void" ? "" : "return "}${inner.code}; })`,
      kind: ["void", "optional"].includes(inner.kind.kind) ? inner.kind : { kind: "optional", inner: inner.kind },
      continueChain: transform => this.optionalChain(receiver, inner.continueChain ? inner.continueChain(transform) : transform(inner)),
    };
  }

  // Module-level lists of definition ids, numeric literals or numeric enum members.
  moduleConstant(node) {
    let symbol = this.checker.getSymbolAtLocation(node);
    if (!symbol) return null;
    if (symbol.flags & ts.SymbolFlags.Alias) symbol = this.checker.getAliasedSymbol(symbol);
    const declaration = symbol.declarations?.[0];
    if (!declaration || !ts.isVariableDeclaration(declaration) || !declaration.initializer) return null;
    if (!ts.isSourceFile(declaration.parent?.parent?.parent)) return null;
    let initializer = declaration.initializer;
    // `const LIST = () => [...]`: a call of it gives the same ids each time.
    const isFunction = ts.isArrowFunction(initializer) && initializer.parameters.length === 0 && !ts.isBlock(initializer.body);
    if (isFunction) initializer = initializer.body;
    while (ts.isAsExpression(initializer) || ts.isSatisfiesExpression(initializer) || ts.isParenthesizedExpression(initializer)) {
      initializer = initializer.expression;
    }
    if (!ts.isArrayLiteralExpression(initializer)) return null;
    const ids = [];
    for (const element of initializer.elements) {
      if (ts.isSpreadElement(element) && ts.isIdentifier(element.expression)) {
        const spread = this.moduleConstant(element.expression);
        if (!spread) return null;
        ids.push(...spread.ids);
        continue;
      }
      const id = ts.isNumericLiteral(element) ? Number(element.text)
        : ts.isIdentifier(element) ? handleIdOfIdentifier(this.checker, element)
        : ts.isPropertyAccessExpression(element) && ts.isIdentifier(element.expression) && ENUM_OBJECTS[element.expression.text]
          ? `${ENUM_OBJECTS[element.expression.text]}::${element.name.text}` : null;
      if (id === null) return null;
      ids.push(id);
    }
    return { code: `std::vector<double>{${ids.join(", ")}} /* ${node.text} */`, kind: { kind: "array", element: { kind: "number" } }, ids, isFunction };
  }

  propertyAccess(node) {
    const name = node.name.text;
    // Variables are a virtual object on handles: translate the whole access before descending into it.
    if (ts.isPropertyAccessExpression(node.expression) && node.expression.name.text === "variables") {
      const owner = this.expression(node.expression.expression);
      const variable = receiver => {
        if (node.expression.questionDotToken && receiver.kind.kind === "optional") {
          return this.optionalChain(receiver, variable({ code: "value", kind: receiver.kind.inner }));
        }
        receiver = this.unwrapOptional(node, receiver);
        return { code: receiver.kind.kind === "anyState"
          ? `js::variableOf(${receiver.code}, ${this.names.use(name)})`
          : `${receiver.code}.getVariable(${this.names.use(name)})`, kind: { kind: "number" } };
      };
      return ts.isPropertyAccessChain(node) && owner.continueChain ? owner.continueChain(variable) : variable(owner);
    }
    // Every segment in x?.a.b remains optional; only parentheses end that chain.
    if (ts.isPropertyAccessChain(node)) {
      const receiver = this.expression(node.expression);
      if (receiver.continueChain) {
        return receiver.continueChain(value => {
          this.receiverOverrides.set(node.expression, value);
          try { return this.propertyAccess(node); }
          finally { this.receiverOverrides.delete(node.expression); }
        });
      }
      if (node.questionDotToken && receiver.kind.kind === "optional") {
        const inner = this.memberOf(node, { code: "value", kind: receiver.kind.inner }, name);
        return this.optionalChain(receiver, inner);
      }
    }
    if (ts.isIdentifier(node.expression) && ENUM_OBJECTS[node.expression.text]) {
      return { code: `${ENUM_OBJECTS[node.expression.text]}::${name}`, kind: { kind: "number" } };
    }
    return this.memberOf(node, this.unwrapOptional(node, this.expression(node.expression)), name);
  }

  // A property of an already translated receiver.
  memberOf(node, receiver, name) {
    switch (receiver.kind.kind) {
      case "context": {
        if (name === "e" || name === "eventArg") {
          // Card code sees the event argument through a mapped (reactive) type; the raw class is on rawEventArg.
          const contextType = this.checker.getTypeAtLocation(node.expression);
          const raw = contextType.getProperty("rawEventArg");
          const rawType = raw ? this.checker.getTypeOfSymbolAtLocation(raw, node) : null;
          const kind = rawType ? { ...valueKind(this.checker, rawType), rawType } : this.kindOf(node);
          if (kind.kind !== "eventArgument") return this.fail(node, `event argument type ${kind.name ?? kind.kind}`);
          return { code: `c.e<${kind.cppClass}>()`, kind };
        }
        if (name === "skillInfo") return { code: "(&c.skillInfo())", kind: { kind: "skillInfo" } };
        if (name === "player") return { code: "c.player()", kind: { kind: "player" } };
        if (name === "oppPlayer") return { code: "c.oppPlayer()", kind: { kind: "player" } };
        const rule = CONTEXT_PROPERTY_RULES[name];
        return rule ? rule(receiver, this.contextInfo) : this.fail(node, `context.${name}`);
      }
      case "character": {
        const rule = CHARACTER_PROPERTY_RULES[name];
        return rule ? rule(receiver) : this.fail(node, `character.${name}`);
      }
      case "entity": {
        const rule = ENTITY_PROPERTY_RULES[name];
        return rule ? rule(receiver) : this.fail(node, `entity.${name}`);
      }
      case "query": return { code: `${receiver.code}.${QUERY_NAMES[name] ?? name}()`, kind: { kind: name === "keys" ? "queryKeys" : "query" } };
      case "queryKeys":
        if (["diceCost", "inInitialPile"].includes(name)) return { code: `${receiver.code}.${name}`, kind: { kind: "string" } };
        return this.fail(node, `query key ${name}`);
      case "eventArgument": return this.eventMember(node, receiver, name);
      case "variableChangeInfo":
        if (["oldValue", "diffValue", "newValue"].includes(name)) return { code: `${receiver.code}.${name}`, kind: { kind: "number" } };
        return this.fail(node, `variableChangeInfo.${name}`);
      case "reactionInfo":
        // Only `fromDamage` is read, as a presence test.
        if (name === "fromDamage") return { code: `(${receiver.code}.fromDamage != nullptr)`, kind: { kind: "boolean" } };
        return this.fail(node, `reactionInfo.${name}`);
      case "json":
        if (name === "length") return { code: `static_cast<double>(${receiver.code}.asArray().size())`, kind: { kind: "number" } };
        return this.fromJson(node, `${receiver.code}.at(${JSON.stringify(name)})`);
      case "set":
        if (name === "size") return { code: `js::length(${receiver.code})`, kind: { kind: "number" } };
        return this.fail(node, `set.${name}`);
      default: {
        const rule = PROPERTY_RULES_BY_KIND[receiver.kind.kind]?.[name];
        return rule ? rule(receiver) : this.fail(node, `${this.describe(receiver.kind)}.${name}`);
      }
    }
  }

  // A part of a JSON value (extension state): numbers and booleans by their TS type, anything else stays JSON.
  fromJson(node, code) {
    const kind = this.kindOf(node);
    if (kind.kind === "number") return { code: `${code}.asNumber()`, kind };
    if (kind.kind === "boolean") return { code: `${code}.asBool()`, kind };
    return { code, kind: { kind: "json" } };
  }

  // Event argument members: states come back as live handles, everything else as the C++ member of that name.
  eventMember(node, receiver, name) {
    if (name === "action" && ["ActionEventArg", "ModifyActionEventArg"].includes(receiver.kind.cppClass)) {
      return { code: `${receiver.code}.action()`, kind: { kind: "actionInfo" } };
    }
    // ReactionEventArg's public `caller` is the reaction's source (EventArg's own caller is protected).
    if (name === "reactionInfo" && ["ReactionEventArg", "ModifyReactionEventArg"].includes(receiver.kind.cppClass)) {
      return { code: `${receiver.code}.reactionInfo`, kind: { kind: "reactionInfo" } };
    }
    if (name === "caller" && ["ReactionEventArg", "ModifyReactionEventArg"].includes(receiver.kind.cppClass)) {
      return { code: `c.get(${receiver.code}.reactionCaller())`, kind: { kind: "anyState" } };
    }
    if (name === "info" && ["VariableEventArg", "BeforeVariableEventArg"].includes(receiver.kind.cppClass)) {
      return { code: `${receiver.code}.info`, kind: { kind: "variableChangeInfo" } };
    }
    if (DAMAGE_INFO_MEMBERS.has(name)) return { code: `${receiver.code}.${name}()`, kind: { kind: "damageInfo" } };
    if (["from", "area", "callerArea"].includes(name)) {
      const field = (EVENT_FIELDS[receiver.kind.cppClass] ?? []).includes(name);
      return { code: field ? `${receiver.code}.${name}` : `${receiver.code}.${name}()`, kind: { kind: "area" } };
    }
    if (receiver.kind.cppClass === "InitiativeSkillArgument" && name === "targets") {
      return { code: `js::handles(c, ${receiver.code}.targets)`, kind: { kind: "array", element: { kind: "anyState" } } };
    }
    const rawProperty = receiver.kind.rawType?.getProperty(name);
    let kind = rawProperty ? valueKind(this.checker, this.checker.getTypeOfSymbolAtLocation(rawProperty, node)) : this.kindOf(node);
    if (kind.kind === "unknown") kind = this.kindOf(node);
    const access = (EVENT_FIELDS[receiver.kind.cppClass] ?? []).includes(name) ? `${receiver.code}.${name}` : `${receiver.code}.${name}()`;
    if (kind.kind === "character") return { code: `c.get(${access}).asCharacter()`, kind };
    if (kind.kind === "entity") return { code: `c.get(${access}).asEntity()`, kind };
    if (kind.kind === "anyState") return { code: `c.get(${access})`, kind };
    if (["number", "boolean", "string", "skillInfo", "switchInfo", "actionInfo", "area"].includes(kind.kind)) return { code: access, kind };
    return this.fail(node, `event ${receiver.kind.cppClass}.${name} (${kind.kind})`);
  }

  call(node) {
    const callee = node.expression;
    // `:callSnippet.name()`: a named snippet without an argument.
    if (ts.isPropertyAccessExpression(callee) && ts.isPropertyAccessExpression(callee.expression) &&
        callee.expression.name.text === "callSnippet" && node.arguments.length === 0 &&
        this.expression(callee.expression.expression).kind.kind === "context") {
      return { code: `c.callSnippet(${JSON.stringify(callee.name.text)}, nullptr)`, kind: { kind: "void" } };
    }
    if (ts.isPropertyAccessExpression(callee) && callee.name.text === "setExtensionState" &&
        this.expression(callee.expression).kind.kind === "context") {
      return this.extensionSetter(node, node.arguments[0]);
    }
    const argumentsTranslated = node.arguments.map((argument) => this.argument(argument));
    if (ts.isPropertyAccessExpression(callee)) {
      const name = callee.name.text;
      if (ts.isIdentifier(callee.expression) && callee.expression.text === "Math" && ["min", "max", "floor", "ceil", "abs"].includes(name)) {
        return { code: `js::math_${name}(${argumentsTranslated.map((argument) => argument.code).join(", ")})`, kind: { kind: "number" } };
      }
      const optionalReceiver = this.expression(callee.expression);
      if (ts.isPropertyAccessChain(callee) && optionalReceiver.continueChain) {
        return optionalReceiver.continueChain(value => {
          this.receiverOverrides.set(callee.expression, value);
          try { return this.call(node); }
          finally { this.receiverOverrides.delete(callee.expression); }
        });
      }
      if (callee.questionDotToken && optionalReceiver.kind.kind === "optional") {
        this.receiverOverrides.set(callee.expression, { code: "value", kind: optionalReceiver.kind.inner });
        const saved = callee.questionDotToken;
        let inner;
        try {
          callee.questionDotToken = undefined;
          inner = this.call(node);
        } finally {
          callee.questionDotToken = saved;
          this.receiverOverrides.delete(callee.expression);
        }
        return this.optionalChain(optionalReceiver, inner);
      }
      const receiver = this.unwrapOptional(callee.expression, optionalReceiver);
      let rule;
      switch (receiver.kind.kind) {
        case "context": {
          const contextRule = CONTEXT_RULES[name] ?? EXTRA_CONTEXT_RULES[name];
          rule = contextRule && ((args) => contextRule(args, this.names));
          break;
        }
        case "character": rule = CHARACTER_RULES[name] && ((args) => CHARACTER_RULES[name](receiver, args, this.names)); break;
        case "entity": rule = ENTITY_RULES[name] && ((args) => ENTITY_RULES[name](receiver, args, this.names)); break;
        case "query": {
          // A live handle used as a query is the query for its id (reactive states are queries in JS).
          const queryArguments = argumentsTranslated.map((argument) =>
            ["character", "entity", "anyState"].includes(argument.kind.kind) ? `query::dollar().id(${argument.code}.id())` : argument.code);
          return { code: `${receiver.code}.${QUERY_NAMES[name] ?? name}(${queryArguments.join(", ")})`, kind: { kind: "query" } };
        }
        case "eventArgument": {
          const adapted = argumentsTranslated.map((argument) => {
            if (SKILL_TYPE_METHODS.has(name) && typeof argument.literal === "string") return SKILL_TYPES[argument.literal] ?? argument.code;
            if (name === "isSkillOrTalentOf" && argument.kind.kind === "character") return `*${argument.code}.latest()`;
            if (TAG_METHODS.has(name) && typeof argument.literal === "string") return `intern(${argument.code})`;
            return argument.code;
          });
          const code = name === "hasOneOfCardTag"
            ? `${receiver.code}.${name}({${adapted.join(", ")}})`
            : `${receiver.code}.${name}(${adapted.join(", ")})`;
          if (name === "getReaction") return { code, kind: { kind: "number" }, presence: `(${code} != reaction::None)` };
          if (name === "isSwirl") return { code, kind: { kind: "number" }, presence: `(${code} >= 0)` };
          return { code, kind: this.kindOf(node) };
        }
        case "set":
          if (name === "has") return { code: `js::includes(${receiver.code}, ${argumentsTranslated[0].code})`, kind: { kind: "boolean" } };
          break;
        default: {
          const kindRule = CALL_RULES_BY_KIND[receiver.kind.kind]?.[name];
          rule = kindRule && ((args) => kindRule(receiver, args, this.kindOf(node)));
          break;
        }
      }
      let result = rule?.(argumentsTranslated);
      // A rule that leaves out an argument would silently change the card: report it instead.
      for (const argument of argumentsTranslated) {
        if (!result || argument.literal !== undefined || argument.kind.kind === "query") continue;
        // Option objects: passed through, or read field by field (the rule then marks its code /*options*/).
        if (argument.kind.kind === "object" && result.code.includes("/*options*/")) continue;
        if (!result.code.includes(argument.code)) result = null;
      }
      if (result && receiver.kind.kind === "context" && (name === "query" || name === "queryAll")) result = this.typedQueryResult(node, result);
      return result ?? this.fail(node, `call ${this.describe(receiver.kind)}.${name}`);
    }
    if (ts.isIdentifier(callee) && argumentsTranslated.length === 0 && this.moduleConstant(callee)?.isFunction) return this.moduleConstant(callee);
    if (ts.isIdentifier(callee) && callee.text === "originalDiceCostOfCard" && argumentsTranslated[0]?.kind.kind === "definition") {
      return { code: `static_cast<double>(originalDiceCostSizeOfCard(*${argumentsTranslated[0].code}))`, kind: { kind: "number" } };
    }
    if (ts.isIdentifier(callee) && callee.text === "flip" && argumentsTranslated.length === 1) {
      return { code: `(1 - ${argumentsTranslated[0].code})`, kind: { kind: "number" } };
    }
    return this.fail(node, "call of a non-member");
  }

  // c.query / c.queryAll give AnyHandles; the checker knows whether they are characters or entities.
  typedQueryResult(node, result) {
    const kind = this.kindOf(node);
    const single = kind.kind === "optional" ? kind.inner : kind.kind === "array" ? kind.element : null;
    if (!single || !["character", "entity"].includes(single.kind)) return result;
    const helper = `${kind.kind === "optional" ? "optional" : "list"}Of${single.kind === "character" ? "Characters" : "Entities"}`;
    return { code: `js::${helper}(${result.code})`, kind };
  }

  argument(node) {
    if (ts.isObjectLiteralExpression(node)) return this.optionsObject(node);
    if (ts.isSpreadElement(node)) return { ...this.expression(node.expression), spread: true };
    return this.expression(node);
  }

  // `:setExtensionState((st) => st.path[i]++)` and `= value` / `+= value`: edits of the JSON extension state.
  extensionSetter(node, setter) {
    if (!setter || !ts.isArrowFunction(setter) || setter.parameters.length !== 1) return this.fail(node, "extension setter");
    const parameter = setter.parameters[0].name.getText();
    let body = setter.body;
    if (ts.isBlock(body)) {
      if (body.statements.length !== 1 || !ts.isExpressionStatement(body.statements[0])) return this.fail(node, "extension setter");
      body = body.statements[0].expression;
    }
    const place = (target) => {
      if (ts.isParenthesizedExpression(target)) return place(target.expression);
      if (ts.isIdentifier(target) && target.text === parameter) return "state";
      if (ts.isPropertyAccessExpression(target)) {
        const owner = place(target.expression);
        return owner && `${owner}[${JSON.stringify(target.name.text)}]`;
      }
      if (ts.isElementAccessExpression(target)) {
        const owner = place(target.expression);
        return owner && `${owner}.asArray().at(static_cast<size_t>(${this.expression(target.argumentExpression).code}))`;
      }
      return null;
    };
    let edit = null;
    if ((ts.isPostfixUnaryExpression(body) || ts.isPrefixUnaryExpression(body)) &&
        [ts.SyntaxKind.PlusPlusToken, ts.SyntaxKind.MinusMinusToken].includes(body.operator)) {
      const target = place(body.operand);
      const step = body.operator === ts.SyntaxKind.PlusPlusToken ? "+ 1" : "- 1";
      if (target) edit = `${target} = JsonValue(${target}.asNumber() ${step})`;
    } else if (ts.isBinaryExpression(body)) {
      const target = place(body.left);
      const value = this.expression(body.right).code;
      if (target && body.operatorToken.kind === ts.SyntaxKind.EqualsToken) edit = `${target} = js::toJson(${value})`;
      if (target && body.operatorToken.kind === ts.SyntaxKind.PlusEqualsToken) edit = `${target} = JsonValue(${target}.asNumber() + ${value})`;
      if (target && body.operatorToken.kind === ts.SyntaxKind.MinusEqualsToken) edit = `${target} = JsonValue(${target}.asNumber() - ${value})`;
    }
    if (!edit) return this.fail(node, "extension setter");
    return { code: `c.setExtensionState([&](JsonValue& state) { ${edit}; })`, kind: { kind: "void" } };
  }

  // Option bags passed to context methods (overrideVariables, kind, ...), as C++ designated initializers.
  optionsObject(node) {
    const raw = {};
    const fields = node.properties.map((property) => {
      if (ts.isPropertyAssignment(property) && !ts.isObjectLiteralExpression(property.initializer)) {
        raw[property.name.getText()] = this.expression(property.initializer).code;
      }
      if (!ts.isPropertyAssignment(property)) return this.fail(property, "options shorthand").code;
      const key = property.name.getText();
      if (key === "overrideVariables" && ts.isObjectLiteralExpression(property.initializer)) {
        const entries = property.initializer.properties.map((entry) => {
          const value = ts.isShorthandPropertyAssignment(entry) ? this.expression(entry.name).code : this.expression(entry.initializer).code;
          return `{${this.names.use(entry.name.getText())}, ${value}}`;
        });
        return `.overrideVariables = {${entries.join(", ")}}`;
      }
      return `.${key} = ${this.expression(property.initializer).code}`;
    });
    return { code: `CreateEntityOptions{${fields.join(", ")}}`, kind: { kind: "object" }, fields: raw };
  }

  binary(node) {
    const operator = node.operatorToken.kind;
    if (operator === ts.SyntaxKind.AsteriskAsteriskToken) {
      return { code: `std::pow(${this.expression(node.left).code}, ${this.expression(node.right).code})`, kind: { kind: "number" } };
    }
    if (operator === ts.SyntaxKind.AmpersandAmpersandToken || operator === ts.SyntaxKind.BarBarToken) {
      const kind = this.kindOf(node);
      if (kind.kind === "boolean") {
        const joiner = operator === ts.SyntaxKind.AmpersandAmpersandToken ? "&&" : "||";
        return { code: `(${this.condition(node.left)} ${joiner} ${this.condition(node.right)})`, kind };
      }
      // `x && flag` with a boolean right side: only its truthiness is ever read (conditions, `when` results).
      if (operator === ts.SyntaxKind.AmpersandAmpersandToken && this.kindOf(node.right).kind === "boolean") {
        return { code: `(${this.condition(node.left)} && ${this.condition(node.right)})`, kind: { kind: "boolean" } };
      }
      return this.fail(node, "&&/|| producing a non-boolean");
    }
    const compound = { [ts.SyntaxKind.PlusEqualsToken]: "+=", [ts.SyntaxKind.MinusEqualsToken]: "-=", [ts.SyntaxKind.AsteriskEqualsToken]: "*=" };
    if (compound[operator] && ts.isIdentifier(node.left)) {
      return { code: `${this.expression(node.left).code} ${compound[operator]} ${this.expression(node.right).code}`, kind: { kind: "void" } };
    }
    if (operator === ts.SyntaxKind.EqualsToken) {
      const left = this.expression(node.left);
      const right = this.expression(node.right);
      // JS stores the value itself; TS typed the variable as present, so an optional result is unwrapped.
      const unwrap = right.kind.kind === "optional" && ts.isIdentifier(node.left) && this.declaredKinds.get(node.left.text) &&
        this.declaredKinds.get(node.left.text).kind !== "optional";
      return { code: `${left.code} = ${unwrap ? `*${right.code}` : right.code}`, kind: { kind: "void" } };
    }
    if (operator === ts.SyntaxKind.PercentToken) {
      return { code: `js::remainder(${this.expression(node.left).code}, ${this.expression(node.right).code})`, kind: { kind: "number" } };
    }
    if (operator === ts.SyntaxKind.QuestionQuestionToken && ts.isElementAccessExpression(node.left)) {
      // TS types array[i] as present; JS reads undefined past the end, so `array[i] ?? x` is an index with a default.
      const array = this.expression(node.left.expression);
      const right = this.expression(node.right);
      if (array.kind.kind === "array") {
        return { code: `js::indexOr(${array.code}, ${this.expression(node.left.argumentExpression).code}, ${right.code})`, kind: array.kind.element };
      }
    }
    if (operator === ts.SyntaxKind.QuestionQuestionToken) {
      const left = this.expression(node.left);
      const right = this.expression(node.right);
      if (left.kind.kind === "optional") return { code: `js::valueOr(${left.code}, ${right.code})`, kind: left.kind.inner };
      return left;
    }
    const isNothing = (side) => side.kind === ts.SyntaxKind.NullKeyword || (ts.isIdentifier(side) && side.text === "undefined");
    if ([ts.SyntaxKind.EqualsEqualsEqualsToken, ts.SyntaxKind.ExclamationEqualsEqualsToken].includes(operator) && isNothing(node.right)) {
      const left = this.expression(node.left);
      const present = left.presence ?? (left.kind.kind === "optional" ? `${left.code}.has_value()` : null);
      if (!present) return this.fail(node, `null comparison of ${this.describe(left.kind)}`);
      return { code: operator === ts.SyntaxKind.EqualsEqualsEqualsToken ? `!${present}` : present, kind: { kind: "boolean" } };
    }
    if ([ts.SyntaxKind.EqualsEqualsEqualsToken, ts.SyntaxKind.ExclamationEqualsEqualsToken].includes(operator)) {
      const left = this.expression(node.left);
      const right = this.expression(node.right);
      if (left.kind.kind === "optional" || right.kind.kind === "optional") {
        const equal = `js::strictEquals(${left.code}, ${right.code})`;
        return { code: operator === ts.SyntaxKind.EqualsEqualsEqualsToken ? equal : `!${equal}`, kind: { kind: "boolean" } };
      }
    }
    const cppOperator = BINARY_OPERATORS[operator];
    if (!cppOperator) return this.fail(node, `binary ${ts.SyntaxKind[operator]}`);
    const left = this.expression(node.left);
    const right = this.expression(node.right);
    const comparesHandles = ["==", "!="].includes(cppOperator) && ["character", "entity"].includes(left.kind.kind);
    const leftCode = comparesHandles ? `${left.code}.id()` : left.code;
    const rightCode = comparesHandles ? `${right.code}.id()` : right.code;
    const kind = ["==", "!=", "<", "<=", ">", ">="].includes(cppOperator) ? { kind: "boolean" } : { kind: "number" };
    return { code: `${leftCode} ${cppOperator} ${rightCode}`, kind };
  }
}
