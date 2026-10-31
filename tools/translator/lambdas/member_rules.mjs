// How members of card-code values become C++: per receiver kind, member name -> rule. A rule gets the translated
// receiver and arguments ({code, kind}) and returns {code, kind}, or null when it can't translate that call.

const number = (code) => ({ code, kind: { kind: "number" } });
const boolean = (code) => ({ code, kind: { kind: "boolean" } });
const voidResult = (code) => ({ code, kind: { kind: "void" } });
const handles = (code) => ({ code, kind: { kind: "array", element: { kind: "anyState" } } });

// Targets for context methods: a query becomes c.queryAll(...); a handle becomes a one-element list.
export function targetsOf(argument) {
  if (!argument) return null;
  switch (argument.kind.kind) {
    case "query": return `c.queryAll(${argument.code})`;
    case "character": return `std::vector<AnyHandle>{AnyHandle(&c, ${argument.code}.id(), true)}`;
    case "entity": return `std::vector<AnyHandle>{AnyHandle(&c, ${argument.code}.id(), false)}`;
    case "anyState":
    case "optional": return `js::handles(c, ${argument.code})`;
    case "array": return `js::handles(c, ${argument.code})`;
    default: return null;
  }
}

function variableName(argument, names) {
  if (argument?.literal === undefined || typeof argument.literal !== "string") return null;
  return names.use(argument.literal);
}

export const CONTEXT_RULES = {
  damage: ([type, value, target]) =>
    target ? voidResult(`c.damage(${type.code}, ${value.code}, ${targetsOf(target)})`) : voidResult(`c.damage(${type.code}, ${value.code})`),
  heal: ([value, target, options]) => {
    if (!options) return voidResult(`c.heal(${value.code}, ${targetsOf(target)})`);
    const kinds = { '"common"': "Common", '"revive"': "Revive", '"immuneDefeated"': "ImmuneDefeated", '"distribution"': "Distribution", '"increaseMaxHealth"': "IncreaseMaxHealth" };
    const kind = kinds[options.fields?.kind];
    if (!kind || Object.keys(options.fields).length !== 1) return null;
    return voidResult(`c.heal(${value.code}, ${targetsOf(target)}, HealKind::${kind}) /*options*/`);
  },
  apply: ([type, target]) => voidResult(`c.apply(${type.code}, ${targetsOf(target)})`),
  gainEnergy: ([value, target]) => voidResult(`c.gainEnergy(${value.code}, ${targetsOf(target)})`),
  switchActive: ([target]) => voidResult(`c.switchActive(${targetsOf(target)})`),
  characterStatus: ([id, target, options]) =>
    voidResult(target ? `c.characterStatus(${id.code}, ${targetsOf(target)}${options ? `, ${options.code}` : ""})` : `c.characterStatus(${id.code})`),
  combatStatus: ([id, where, options]) =>
    ({ code: `c.combatStatus(${id.code}${where ? `, ${where.literal === "opp"}` : ""}${options ? `, ${options.code}` : ""})`, kind: { kind: "optional", inner: { kind: "entity" } } }),
  summon: ([id, where, options]) =>
    ({ code: `c.summon(${id.code}${where ? `, ${where.literal === "opp"}` : ""}${options ? `, ${options.code}` : ""})`, kind: { kind: "optional", inner: { kind: "entity" } } }),
  dispose: ([target, options]) => {
    if (!options) return voidResult(target ? `c.dispose(${targetsOf(target)})` : "c.dispose()");
    const reasons = { '"discarded"': "Discarded", '"overflow"': "Overflow", '"other"': "Other", '"elementalTuning"': "ElementalTuning" };
    const fields = options.fields ?? {};
    if (Object.keys(fields).some((key) => key !== "reason" && key !== "direct")) return null;
    const reason = fields.reason ? reasons[fields.reason] : "Other";
    if (!reason) return null;
    const targets = target ? targetsOf(target) : "std::nullopt";
    return voidResult(`c.dispose(${targets}, RemoveReason::${reason}, ${fields.direct ?? "false"}) /*options*/`);
  },
  getVariable: ([name, target], names) => {
    const variable = variableName(name, names);
    if (!variable) return null;
    return number(target ? `c.getVariable(${variable}, js::handle(c, ${target.code}))` : `c.getVariable(${variable})`);
  },
  setVariable: ([name, value, target], names) => {
    const variable = variableName(name, names);
    if (!variable) return null;
    return voidResult(target ? `c.setVariable(${variable}, ${value.code}, js::handle(c, ${target.code}))` : `c.setVariable(${variable}, ${value.code})`);
  },
  addVariable: ([name, value, target], names) => {
    const variable = variableName(name, names);
    if (!variable) return null;
    return voidResult(target ? `c.addVariable(${variable}, ${value.code}, js::handle(c, ${target.code}))` : `c.addVariable(${variable}, ${value.code})`);
  },
  consumeUsage: ([count, target]) =>
    voidResult(target ? `c.consumeUsage(${count.code}, js::handle(c, ${target.code}))` : `c.consumeUsage(${count?.code ?? 1})`),
  consumeUsagePerRound: ([count]) => voidResult(`c.consumeUsagePerRound(${count?.code ?? 1})`),
  query: ([query]) => (query.kind.kind === "query" ? { code: `c.query(${query.code})`, kind: { kind: "optional", inner: { kind: "anyState" } } } : null),
  queryAll: ([query]) => (query.kind.kind === "query" ? handles(`c.queryAll(${query.code})`) : null),
  useSkill: ([skill, options]) => {
    if (!options) return voidResult(skill.literal === "normal" ? "c.useNormalSkill()" : `c.useSkill(${skill.code})`);
    const fields = options?.fields ?? {};
    if (options && Object.keys(fields).some((key) => key !== "asPrepared")) return null;
    const prepared = fields.asPrepared ?? "false";
    return voidResult((skill.literal === "normal" ? `c.useNormalSkill(${prepared})` : `c.useSkill(${skill.code}, ${prepared})`) + (options ? " /*options*/" : ""));
  },
  drawCards: ([count, options]) => {
    if (count.kind.kind !== "number") return null;
    if (!options) return voidResult(`c.drawCards(${count.code})`);
    const fields = options.fields ?? {};
    const tag = fields.withTag ? `intern(${fields.withTag})` : "std::nullopt";
    if (Object.keys(fields).some((key) => !["who", "withTag", "withAttachment", "withDefinition"].includes(key))) return null;
    return voidResult(`c.drawCards(${count.code}, ${fields.who === '"opp"'}, ${tag}, ${fields.withAttachment ?? "std::nullopt"}, ${fields.withDefinition ?? "std::nullopt"}) /*options*/`);
  },
  generateDice: ([type, count]) =>
    type.literal === "randomElement" ? voidResult(`c.generateRandomElementDice(${count.code})`) : voidResult(`c.generateDice(${type.code}, ${count.code})`),
  createHandCard: ([id, where]) => ({ code: `c.createHandCard(${id.code}${where ? `, ${where.literal === "opp"}` : ""})`, kind: { kind: "optional", inner: { kind: "entity" } } }),
  // JS items[random % 0] is undefined for an empty list: an optional here.
  random: ([items]) => (items.kind.kind === "array" ? { code: `js::randomOf(c, ${items.code})`, kind: { kind: "optional", inner: items.kind.element } } : null),
  increaseMaxHealth: ([value, target, options]) => {
    if (!options) return voidResult(`c.increaseMaxHealth(${value.code}, ${targetsOf(target)})`);
    const fields = options.fields ?? {};
    if (Object.keys(fields).some((key) => key !== "heal")) return null;
    return voidResult(`c.increaseMaxHealth(${value.code}, ${targetsOf(target)}, ${fields.heal ?? "true"}) /*options*/`);
  },
  immune: ([value]) => voidResult(`c.immune(${value.code})`),
  transformDefinition: ([target, id]) => voidResult(`c.transformDefinition(${targetsOf(target)}, ${id.code})`),
  gainNightsoul: ([target, count]) => voidResult(`c.gainNightsoul(${targetsOf(target)}${count ? `, ${count.code}` : ""})`),
  consumeNightsoul: ([target, count]) => voidResult(`c.consumeNightsoul(${targetsOf(target)}${count ? `, ${count.code}` : ""})`),
  countOfSkill: (args) => (args.length === 0 ? number("c.countOfSkill()") : number(`c.countOfSkill(${args[0].code}, ${args[1].code})`)),
  isMyTurn: () => boolean("c.isMyTurn()"),
  // A live handle is already "latest".
  get: ([state]) => (["entity", "character", "anyState"].includes(state?.kind.kind) ? state : null),
  abortPreview: () => voidResult("c.abortPreview()"),
  eventBoundary: () => voidResult("c.eventBoundary()"),
  swapPlayerHandCards: () => voidResult("c.swapPlayerHandCards()"),
  // `:callSnippet()` runs the default snippet; named snippets are `:callSnippet.name(arg)`.
  callSnippet: (args) => (args.length === 0 ? voidResult('c.callSnippet("default", nullptr)') : null),
  rerollDice: ([times]) => voidResult(`c.rerollDice(${times.code})`),
  switchCards: () => voidResult("c.switchCards()"),
  continueNextTurn: ([who]) => voidResult(`c.continueNextTurn(${who ? who.literal === "opp" : false})`),
  adventure: () => voidResult("c.adventure()"),
  finishAdventure: () => voidResult("c.finishAdventure()"),
};

export const CONTEXT_PROPERTY_RULES = {
  self: (_, contextInfo) => ({ code: contextInfo.selfCode, kind: contextInfo.selfKind }),
  isPreview: () => boolean("c.isPreview()"),
  roundNumber: () => number("c.roundNumber()"),
};

export const CHARACTER_RULES = {
  isActive: (receiver) => boolean(`${receiver.code}.isActive()`),
  isMine: (receiver) => boolean(`${receiver.code}.isMine()`),
  hasStatus: (receiver, [id]) => ({ code: `${receiver.code}.hasStatus(${id.code})`, kind: { kind: "optional", inner: { kind: "entity" } } }),
  nationTags: (receiver) => ({ code: `js::nameTexts(${receiver.code}.nationTags())`, kind: { kind: "array", element: { kind: "string" } } }),
  hasEquipment: (receiver, [id]) => ({ code: `${receiver.code}.hasEquipment(${id.code})`, kind: { kind: "optional", inner: { kind: "entity" } } }),
  element: (receiver) => number(`${receiver.code}.element()`),
  fullEnergy: (receiver) => boolean(`${receiver.code}.fullEnergy()`),
  gainEnergy: (receiver, [value]) => voidResult(`${receiver.code}.gainEnergy(${value?.code ?? 1})`),
  heal: (receiver, [value, options]) => {
    if (!options) return voidResult(`${receiver.code}.heal(${value.code})`);
    return CONTEXT_RULES.heal([value, receiver, options]);
  },
  damage: (receiver, [type, value]) => voidResult(`${receiver.code}.damage(${type.code}, ${value.code})`),
  addStatus: (receiver, [id, options]) => voidResult(`${receiver.code}.addStatus(${id.code}${options ? `, ${options.code}` : ""})`),
  latest: (receiver) => receiver,
  cast: (receiver) => receiver,
  unequipWeapon: (receiver) => voidResult(`${receiver.code}.unequipWeapon()`),
  unequipArtifact: (receiver) => voidResult(`${receiver.code}.unequipArtifact()`),
  hasNightsoulsBlessing: (receiver) => ({ code: `${receiver.code}.hasNightsoulsBlessing()`, kind: { kind: "optional", inner: { kind: "entity" } } }),
  hasArtifact: (receiver) => ({ code: `${receiver.code}.hasArtifact()`, kind: { kind: "optional", inner: { kind: "entity" } } }),
  hasWeapon: (receiver) => ({ code: `${receiver.code}.hasWeapon()`, kind: { kind: "optional", inner: { kind: "entity" } } }),
  hasTechnique: (receiver) => ({ code: `${receiver.code}.hasTechnique()`, kind: { kind: "optional", inner: { kind: "entity" } } }),
  isSkillDisabled: (receiver) => boolean(`${receiver.code}.isSkillDisabled()`),
  setVariable: (receiver, [name, value], names) =>
    typeof name.literal === "string" ? voidResult(`${receiver.code}.setVariable(${names.use(name.literal)}, ${value.code})`) : null,
  addVariable: (receiver, [name, value], names) =>
    typeof name.literal === "string" ? voidResult(`${receiver.code}.addVariable(${names.use(name.literal)}, ${value.code})`) : null,
  getVariable: (receiver, [name], names) =>
    typeof name.literal === "string" ? number(`${receiver.code}.getVariable(${names.use(name.literal)})`) : null,
  loseEnergy: (receiver, [count]) => number(`${receiver.code}.loseEnergy(${count?.code ?? 1})`),
  weaponTag: (receiver) => ({ code: `nameText(${receiver.code}.weaponTag())`, kind: { kind: "string" } }),
};

export const CHARACTER_PROPERTY_RULES = {
  id: (receiver) => number(`${receiver.code}.id()`),
  health: (receiver) => number(`${receiver.code}.health()`),
  energy: (receiver) => number(`${receiver.code}.energy()`),
  maxHealth: (receiver) => number(`${receiver.code}.maxHealth()`),
  maxEnergy: (receiver) => number(`${receiver.code}.maxEnergy()`),
  aura: (receiver) => number(`${receiver.code}.aura()`),
  who: (receiver) => number(`${receiver.code}.who()`),
  definition: (receiver) => ({ code: `${receiver.code}.definition()`, kind: { kind: "definition", which: "CharacterDefinition" } }),
};

export const ENTITY_RULES = {
  latest: (receiver) => receiver,
  cast: (receiver) => receiver,
  diceCost: (receiver) => number(`${receiver.code}.diceCost()`),
  withAttachment: (receiver, [id]) => boolean(`${receiver.code}.withAttachment(${id.code})`),
  empowered: (receiver) => boolean(`${receiver.code}.empowered()`),
  dispose: (receiver) => voidResult(`${receiver.code}.dispose()`),
  getVariable: (receiver, [name], names) => {
    const variable = variableName(name, names);
    return variable ? number(`${receiver.code}.getVariable(${variable})`) : null;
  },
  addVariable: (receiver, [name, value], names) => {
    const variable = variableName(name, names);
    return variable ? voidResult(`${receiver.code}.addVariable(${variable}, ${value.code})`) : null;
  },
  setVariable: (receiver, [name, value], names) => {
    const variable = variableName(name, names);
    return variable ? voidResult(`${receiver.code}.setVariable(${variable}, ${value.code})`) : null;
  },
  consumeUsage: (receiver, [count]) => voidResult(`${receiver.code}.consumeUsage(${count?.code ?? 1})`),
  isMine: (receiver) => boolean(`${receiver.code}.isMine()`),
};

export const ENTITY_PROPERTY_RULES = {
  area: (receiver) => ({ code: `${receiver.code}.area()`, kind: { kind: "area" } }),
  id: (receiver) => number(`${receiver.code}.id()`),
  who: (receiver) => number(`${receiver.code}.who()`),
  master: (receiver) => ({ code: `${receiver.code}.master()`, kind: { kind: "character" } }),
  definition: (receiver) => ({ code: `${receiver.code}.definition()`, kind: { kind: "definition", which: "EntityDefinition" } }),
};
