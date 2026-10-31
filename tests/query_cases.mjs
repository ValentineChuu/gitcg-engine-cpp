// Real-$ differential cases plus raw numerical S-expressions and seeded combinations.
// Usage: node tests/query_cases.mjs [--states ... --output ... --random 100]
import { appendFileSync, mkdirSync, readFileSync, renameSync, writeFileSync } from "node:fs";
import { parseArgs } from "node:util";
import { $, queryToExpression, runQuery, runSExprQuery } from "@gi-tcg/core";
import getData from "@gi-tcg/data";
import { seededRandom } from "../reference/bridge/seeded_random.mjs";
import { restoreState } from "./query_state_io.mjs";

const { values: options } = parseArgs({ options: {
  states: { type: "string", default: new URL("query_generated/states.json", import.meta.url).pathname },
  output: { type: "string", default: new URL("query_generated/cases.jsonl", import.meta.url).pathname },
  random: { type: "string", default: "100" }, seed: { type: "string", default: "17" },
} });
const data = getData(), random = seededRandom(Number(options.seed));
const dump = JSON.parse(readFileSync(options.states, "utf8"));
const snapshots = dump.snapshots;
const chain = (...steps) => ({ steps: steps.map((step) => typeof step === "string" ? [step] : step) });
const operand = (query) => ({ query });
const key = (name) => ({ key: name });
const fn = (code, scalar = false) => ({ function: code, scalar });
function build(recipe) {
  let query = $;
  for (const [name, ...args] of recipe.steps) {
    const resolved = args.map((arg) => arg?.query ? build(arg.query) : arg?.key ? $.keys[arg.key]
      : arg?.function ? (new Function(`return (${arg.function})`))() : arg);
    if (name === "macro") query = $.macros[args[0]];
    else {
      const method = query[name];
      query = args.length || ["var", "cost", "id", "def", "tag", "tagOf", "orderBy", "orderByFn", "orderByRaw", "limit", "union", "intersection", "exclude", "orElse"].includes(name)
        ? Reflect.apply(method, query, resolved) : method;
    }
  }
  return query;
}
function hasCost(expression) {
  return Array.isArray(expression) && (expression[0] === "special:diceCost" || expression.some(hasCost)
    || expression[0] === "fn" && expression[1].includes("diceCost"));
}
function hasAttachment(state) {
  return state.versionBehavior.diceCostApplyAttachments && state.players.some((p) =>
    [...p.characters.flatMap((c) => c.entities), ...p.supports, ...p.hands, ...p.pile]
      .some((e) => e.attachments?.length));
}

// Explicit edge fixtures supplement sampled games, including attachments, all entity areas and cyclic proximity ties.
const edge = structuredClone(snapshots.find((s) => s.state.phase === "action") ?? snapshots[0]);
let nextId = -900000;
const makeEntity = (definition) => ({ id: nextId--, definition: definition.id, type: definition.type,
  variables: Object.fromEntries(Object.entries(definition.varConfigs).map(([k, v]) => [k, v.initialValue])),
  ...(definition.type === "attachment" ? {} : { attachments: [] }) });
for (const p of edge.state.players) {
  for (const c of p.characters) { c.variables.alive = 1; c.variables.health = 10; c.entities = []; }
  p.activeCharacterId = p.characters[1].id;
  for (const [area, type] of [["combatStatuses", "combatStatus"], ["summons", "summon"], ["supports", "support"]]) {
    p[area] = [...data.entities.values()].filter((e) => e.type === type).slice(0, 3).map(makeEntity);
  }
  for (const type of ["status", "equipment"]) {
    p.characters[0].entities.push(makeEntity([...data.entities.values()].find((e) => e.type === type)));
  }
  p.hands = [...data.entities.values()].filter((e) => ["eventCard", "support", "equipment"].includes(e.type)).slice(0, 5).map(makeEntity);
  p.hands[0].attachments = [...data.attachments.values()].slice(0, 2).map(makeEntity);
  p.pile = structuredClone(p.hands).map((e) => ({ ...e, id: nextId--, attachments: e.attachments.map((a) => ({ ...a, id: nextId-- })) }));
  p.initialPile = [p.hands[1].definition];
  p.removedEntities = [makeEntity([...data.entities.values()].find((e) => e.type === "summon"))];
  p.removedEntities[0].attachments = [...data.attachments.values()].slice(0, 1).map(makeEntity);
  p.characters[0].variables.health = 2;
  p.characters[2].variables.health = 5;
}
const edges = [];
for (let turn = 0; turn < 2; ++turn) for (const dead of [-1, 0, 1, 2]) {
  const fixture = structuredClone(edge);
  fixture.game = "edge"; fixture.decision = `${turn}:${dead}`; fixture.state.currentTurn = turn;
  if (dead >= 0) fixture.state.players[turn].characters[dead].variables.alive = 0;
  edges.push(fixture);
}
const emptySide = structuredClone(edge);
emptySide.game = "edge"; emptySide.decision = "all-opponents-defeated";
for (const c of emptySide.state.players[1].characters) c.variables.alive = 0;
edges.push(emptySide);
const edgePath = new URL("query_generated/edge_states.json", import.meta.url).pathname;
mkdirSync(new URL("query_generated/", import.meta.url), { recursive: true });
writeFileSync(edgePath, JSON.stringify({ snapshots: edges }) + "\n");
const allSnapshots = [...snapshots, ...edges];
const coverage = {}, skipped = {};
let warnings = 0;
console.warn = () => { ++warnings; };
console.trace = () => {};
const temporaryOutput = options.output + ".tmp";
writeFileSync(temporaryOutput, JSON.stringify({ kind: "metadata", states: snapshots.length, edgeStates: edges.length, seed: Number(options.seed) }) + "\n");
let total = 0;
for (let index = 0; index < allSnapshots.length; ++index) {
  const state = restoreState(allSnapshots[index].state, data);
  const pool = [];
  function add(name, recipe) { pool.push({ name, recipe }); }
  const getters = ["any", "my", "opp", "onStage", "offStage", "character", "combatStatus", "summon", "support", "attachment",
    "hand", "pile", "vCharacter", "vHand", "vPile", "typeEquipment", "typeSupport", "typeStatus", "typeEventCard",
    "equipped", "equipment", "status", "eventCard", "active", "prev", "next", "standby", "onlyDefeated", "includesDefeated", "notInitial"];
  for (const getter of getters) {
    // Conservatively omit position searches on all-defeated fixtures; the raw JS helper can loop there.
    if (["next", "prev"].includes(getter) && state.players.some((p) => p.characters.every((c) => !c.variables.alive))) continue;
    add(getter, chain(getter));
    if (getter !== "any") add(`${getter}:my`, chain("my", getter));
  }
  for (const ch of state.players.flatMap((p) => p.characters)) {
    add("id", chain(["id", ch.id]));
    add("def", chain("opp", "character", ["def", ch.definition.id]));
    for (const tag of ch.definition.tags) add("tag", chain("character", ["tag", tag]));
    for (const category of ["weapon", "element"]) add("tagOf", chain("character", ["tagOf", category, operand(chain(["id", ch.id]))]));
    add("recentOppFrom", chain(["recentOppFrom", operand(chain(["id", ch.id]))]));
  }
  add("id-missing", chain(["id", 123456789]));
  add("id-fractional", chain(["id", state.players[0].characters[0].id + 0.5]));
  add("def-fractional", chain(["def", state.players[0].characters[0].definition.id + 0.5]));
  add("id-outside-int32", chain(["id", 2 ** 40]));
  for (const p of state.players) {
    for (const [area, getter] of [["hands", "hand"], ["pile", "pile"], ["summons", "summon"], ["supports", "support"], ["combatStatuses", "combatStatus"]]) {
      const e = p[area][0];
      if (!e) continue;
      add("id-entity", chain(["id", e.id]));
      add("def-entity-area", chain(getter, ["def", e.definition.id]));
      if (e.attachments?.[0]) add("id-attachment", chain(["id", e.attachments[0].id]));
    }
    if (p.removedEntities[0]) add("id-removed", chain(["id", p.removedEntities[0].id]));
    for (const c of p.characters) if (c.entities[0]) {
      add("id-on-character", chain(["id", c.entities[0].id]));
      add("card-pattern-at", chain("equipped", ["at", operand(chain(["id", c.id]))]));
      add("card-pattern-has", chain("character", ["has", operand(chain(["def", c.entities[0].definition.id]))]));
    }
  }
  add("tag-many", chain("character", ["tag", "sword", "hydro"]));
  for (const op of ["=", "!=", "<", "<=", ">", ">="]) {
    add("var-number", chain("character", ["var", "health", op, 5]));
    add("var-variable", chain("character", ["var", "health", op, "maxHealth"]));
    add("var-missing", chain("any", ["var", "missingVariable", op, 0]));
    add("cost-op", chain("offStage", ["cost", op, 2]));
  }
  add("var-value", chain("character", ["var", "alive", 1]));
  add("var-fn", chain("character", ["var", fn("v => v.health < v.maxHealth && v.energy >= 0")]));
  for (const code of ["v => v.missing === undefined", "v => v.missing === v.missing", "v => Number.isNaN(v.missing)",
    "v => Number.isNaN(Number(v.missing))", "v => Number.isFinite(v.missing)", "v => v.alive === true",
    "v => v.alive == true", "v => v.missing == null", "v => !v.missing || v.health > 0"]) {
    add("var-fn-primitives", chain("any", ["var", fn(code)]));
  }
  add("var-key-fn", chain("any", ["var", "health", fn("x => x > 3", true)]));
  add("var-special-fn", chain("hand", ["var", key("inInitialPile"), fn("x => !x", true)]));
  add("cost", chain("hand", ["cost", 0]));
  add("cost-fn", chain("hand", ["cost", fn("x => x >= 2", true)]));
  for (const [relation, target] of [["has", "typeStatus"], ["at", "character"], ["with", "attachment"], ["on", "hand"],
    ["not", "my"], ["recentOppFrom", "active"]]) {
    add(`unary:${relation}:call`, chain([relation, operand(chain(target))]));
    add(`unary:${relation}:chain`, chain(relation, target));
    if (["has", "at", "with", "on"].includes(relation)) add(`relation:${relation}`, chain("my", [relation, operand(chain(target))]));
  }
  for (const binary of ["union", "intersection", "exclude", "orElse"]) {
    add(`binary:${binary}`, chain("my", [binary, operand(chain("opp"))]));
    add(`binary:${binary}:nested`, chain("my", [binary, operand(chain("character"))], ["intersection", operand(chain("active"))]));
  }
  for (const binary of ["union", "intersection"]) {
    add(`${binary}:empty`, chain([binary]));
    add(`${binary}:variadic`, chain([binary, operand(chain("my")), operand(chain("character")), operand(chain("active"))]));
  }
  for (const name of Object.keys($.macros)) add(`macro:${name}`, chain(["macro", name]));
  for (const count of [0, 1, 2, 100, -1, -100, 2.7]) add("limit", chain("character", ["limit", count]));
  for (const op of ["+", "-", "*", "/", "%"]) add("orderBy-arithmetic", chain("character", ["orderBy", "health", op, "maxHealth"], ["limit", 2]));
  add("orderBy", chain("character", ["orderBy", "health"], ["orderBy", "energy"]));
  add("orderBy-key", chain("hand", ["orderBy", key("inInitialPile")]));
  add("orderByFn", chain("character", ["orderByFn", fn("v => Math.max(0, v.maxHealth - v.health)")]));
  add("orderByFn-block", chain("character", ["orderByFn", fn("function(v) { return v.health > 5 ? -v.energy : v.health; }")]));
  add("orderByRaw", chain("character", ["orderByRaw", ["expr", ["-", "health"]], ["expr", "energy"]]));
  for (const variable of ["health", "energy", "usage", "missingVariable"]) {
    add("orderBy-missing-mixed", chain("any", ["orderBy", variable]));
    add("orderBy-missing-descending", chain("any", ["orderBy", 0, "-", variable]));
  }

  // Seeded random nested combinations; choose only stable, unordered builders as operands.
  const bases = pool.filter((item) => !item.name.startsWith("macro:") && !/orderBy|limit|cost/.test(item.name));
  const pick = (values) => values[Math.floor(random() * values.length)];
  for (let n = 0; n < Number(options.random); ++n) {
    const left = pick(bases).recipe, right = pick(bases).recipe, third = pick(bases).recipe;
    const recipe = { steps: [...left.steps, [pick(["union", "intersection", "exclude", "orElse"]), operand(right)],
      [pick(["union", "intersection", "exclude", "orElse"]), operand(third)]] };
    add("random-combination", recipe);
  }
  const records = [];
  const record = (name, expression, recipe, outcome, who) => {
    records.push(JSON.stringify({ state: index, who, name, expression, ...(recipe ? { recipe } : {}), ...outcome }));
    coverage[name] = (coverage[name] ?? 0) + 1; ++total;
  };
  for (const { name, recipe } of pool) {
    let query, expression;
    try { query = build(recipe); expression = queryToExpression(query); }
    catch (error) { throw Error(`Building ${name}: ${JSON.stringify(recipe)}: ${error.message}`); }
    if (hasCost(expression) && hasAttachment(state)) { skipped.attachmentCost = (skipped.attachmentCost ?? 0) + 2; continue; }
    for (const who of [0, 1]) {
      let outcome;
      try { outcome = { expected: runQuery(state, who, query).map((e) => e.id) }; }
      catch (error) { outcome = { expectedError: error.message }; }
      record(name, expression, recipe, outcome, who);
    }
  }
  // The builder's orderByRaw accepts every numerical operator, including empty variadic expressions.
  const numerical = [["+"], ["*"], ["min"], ["max"], ["-", "health"], ["/", "health"],
    ["/", 0, 0], ["/", 1, 0], ["%", -5, 2], ["min", "health", "energy"], ["max", "health", "energy"],
    ["and"], ["or"], ["not", ["=", "health", 0]], ["and", [">", "health", 3], ["<", "energy", 2]],
    ["or", ["=", "health", 1], ["!=", "missing", 0]], ["+", "missing", 1],
    ["special:inInitialPile"], ["special:diceCost"],
    ["<", ["/", ["min", ["-", 0], 0]], 0], [">", ["/", ["max", 0, ["-", 0]]], 0]];
  for (const expr of numerical) for (const who of [0, 1]) {
    const expression = ["intersection", ["defeated", "ignore"], ["variables", ["expr", expr]]];
    if (hasCost(expression) && hasAttachment(state)) { skipped.attachmentCost = (skipped.attachmentCost ?? 0) + 1; continue; }
    record(`numerical:${expr[0]}`, expression, null, { expected: runSExprQuery(state, who, expression).map((e) => e.id) }, who);
  }
  appendFileSync(temporaryOutput, records.join("\n") + "\n");
}
appendFileSync(temporaryOutput, JSON.stringify({ kind: "summary", cases: total }) + "\n");
renameSync(temporaryOutput, options.output);
writeFileSync(new URL("query_generated/coverage.json", import.meta.url), JSON.stringify({ states: allSnapshots.length, cases: total, skipped, warnings, coverage }, null, 2) + "\n");
console.log(`${allSnapshots.length} states (${edges.length} edge fixtures), ${total} cases; skipped ${JSON.stringify(skipped)} -> ${options.output}`);
