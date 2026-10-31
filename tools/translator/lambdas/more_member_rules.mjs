// Member rules for the less common value kinds: mixed character/entity states, definitions, the player view,
// skill/switch/action infos from event arguments, and arrays. Same shape as member_rules.mjs.

import { targetsOf } from "./member_rules.mjs";

const number = (code) => ({ code, kind: { kind: "number" } });
const boolean = (code) => ({ code, kind: { kind: "boolean" } });
const string = (code) => ({ code, kind: { kind: "string" } });
const voidResult = (code) => ({ code, kind: { kind: "void" } });
const voidResultBoolean = (code) => ({ code, kind: { kind: "boolean" } });

// C++ element type of an array kind.
export function cppTypeOf(kind) {
  switch (kind.kind) {
    case "number": return "double";
    case "boolean": return "bool";
    case "string": return "std::string";
    case "character": return "CharacterHandle";
    case "entity": return "EntityHandle";
    case "anyState": return "AnyHandle";
    case "definition": return kind.which === "CharacterDefinition" ? "const CharacterDefinition*" : "const EntityDefinition*";
    default: return null;
  }
}

export const ANY_STATE_PROPERTY_RULES = {
  // Only characters have health; only entities have a master.
  health: (receiver) => number(`${receiver.code}.asCharacter().health()`),
  master: (receiver) => ({ code: `${receiver.code}.asEntity().master()`, kind: { kind: "character" } }),
  id: (receiver) => number(`${receiver.code}.id()`),
  who: (receiver) => number(`js::who(${receiver.code})`),
  definition: (receiver) => ({ code: `js::definitionOf(${receiver.code})`, kind: { kind: "definition", which: "AnyDefinition" } }),
  area: (receiver) => ({ code: `js::areaOf(${receiver.code})`, kind: { kind: "area" } }),
};

export const ANY_STATE_RULES = {
  cast: (receiver) => receiver,
  latest: (receiver) => receiver,
  isMine: (receiver) => boolean(`js::isMine(${receiver.code})`),
  // Only characters carry statuses.
  element: (receiver) => number(`${receiver.code}.asCharacter().element()`),
  hasEquipment: (receiver, [id]) => ({ code: `${receiver.code}.asCharacter().hasEquipment(${id.code})`, kind: { kind: "optional", inner: { kind: "entity" } } }),
  addVariable: (receiver, [name, value]) =>
    typeof name.literal === "string" ? voidResult(`c.addVariable(intern(${name.code}), ${value.code}, ${receiver.code})`) : null,
  hasStatus: (receiver, [id]) => ({ code: `${receiver.code}.asCharacter().hasStatus(${id.code})`, kind: { kind: "optional", inner: { kind: "entity" } } }),
};

export const DEFINITION_PROPERTY_RULES = {
  id: (receiver) => number(`js::idOf(${receiver.code})`),
  type: (receiver) => string(`js::typeNameOf(${receiver.code})`),
  tags: (receiver) => ({ code: `js::tagsOf(${receiver.code})`, kind: { kind: "tags" } }),
  skillType: (receiver) => string(`js::skillTypeNameOf(${receiver.code})`),
};

export const TAGS_RULES = {
  includes: (receiver, [tag]) =>
    tag.literal !== undefined ? boolean(`js::hasTag(${receiver.code}, ${JSON.stringify(tag.literal)})`)
      : tag.kind.kind === "string" ? boolean(`js::hasTag(${receiver.code}, std::string(${tag.code}).c_str())`) : null,
};

export const PLAYER_PROPERTY_RULES = {
  hands: (receiver) => ({ code: `js::entityHandles(c, ${receiver.code}.hands)`, kind: { kind: "array", element: { kind: "entity" } } }),
  pile: (receiver) => ({ code: `js::entityHandles(c, ${receiver.code}.pile)`, kind: { kind: "array", element: { kind: "entity" } } }),
  summons: (receiver) => ({ code: `js::entityHandles(c, ${receiver.code}.summons)`, kind: { kind: "array", element: { kind: "entity" } } }),
  supports: (receiver) => ({ code: `js::entityHandles(c, ${receiver.code}.supports)`, kind: { kind: "array", element: { kind: "entity" } } }),
  combatStatuses: (receiver) => ({ code: `js::entityHandles(c, ${receiver.code}.combatStatuses)`, kind: { kind: "array", element: { kind: "entity" } } }),
  characters: (receiver) => ({ code: `js::characterHandles(c, ${receiver.code}.characters)`, kind: { kind: "array", element: { kind: "character" } } }),
  dice: (receiver) => ({ code: `js::numbers(${receiver.code}.dice)`, kind: { kind: "array", element: { kind: "number" } } }),
  activeCharacterId: (receiver) => number(`${receiver.code}.activeCharacterId`),
  declaredEnd: (receiver) => boolean(`${receiver.code}.declaredEnd`),
  legendUsed: (receiver) => boolean(`${receiver.code}.legendUsed`),
  hasDefeated: (receiver) => boolean(`${receiver.code}.hasDefeated`),
  canPlunging: (receiver) => boolean(`${receiver.code}.canPlunging`),
  who: (receiver) => number(`${receiver.code}.who`),
  initialPile: (receiver) => ({ code: `${receiver.code}.initialPile`, kind: { kind: "array", element: { kind: "definition", which: "EntityDefinition" } } }),
};

export const DAMAGE_INFO_PROPERTY_RULES = {
  type: (receiver) => number(`${receiver.code}.type`),
  value: (receiver) => number(`${receiver.code}.value`),
  causeDefeated: (receiver) => boolean(`${receiver.code}.causeDefeated`),
  isSkillMainDamage: (receiver) => boolean(`${receiver.code}.isSkillMainDamage`),
  fromReaction: (receiver) => ({ code: `${receiver.code}.fromReaction`, kind: { kind: "number" }, presence: `(${receiver.code}.fromReaction != reaction::None)` }),
  via: (receiver) => ({ code: `${receiver.code}.via`, kind: { kind: "skillInfo" } }),
  source: (receiver) => ({ code: `c.get(${receiver.code}.source)`, kind: { kind: "anyState" } }),
  target: (receiver) => ({ code: `CharacterHandle(&c, ${receiver.code}.target->id)`, kind: { kind: "character" } }),
  expectedValue: (receiver) => number(`${receiver.code}.expectedValue`),
};

export const SKILL_INFO_PROPERTY_RULES = {
  definition: (receiver) => ({ code: `${receiver.code}->definition`, kind: { kind: "skillDefinition" } }),
  caller: (receiver) => ({ code: `c.get(${receiver.code}->caller)`, kind: { kind: "anyState" } }),
  charged: (receiver) => boolean(`${receiver.code}->charged`),
  plunging: (receiver) => boolean(`${receiver.code}->plunging`),
  prepared: (receiver) => boolean(`${receiver.code}->prepared`),
  requestBy: (receiver) => ({ code: `${receiver.code}->requestBy`, kind: { kind: "skillInfo" } }),
};

export const SKILL_DEFINITION_PROPERTY_RULES = {
  id: (receiver) => number(`${receiver.code}->id`),
  skillType: (receiver) => string(`js::skillTypeName(${receiver.code}->skillType)`),
  initiativeSkillConfig: (receiver) => ({ code: `${receiver.code}->initiativeSkillConfig`, kind: { kind: "optional", inner: { kind: "initiativeSkillConfig" } } }),
};

export const INITIATIVE_SKILL_CONFIG_PROPERTY_RULES = {
  computed$costSize: (receiver) => number(`static_cast<double>(${receiver.code}.costSize)`),
  computed$diceCostSize: (receiver) => number(`static_cast<double>(${receiver.code}.diceCostSize)`),
};

export const SWITCH_INFO_PROPERTY_RULES = {
  from: (receiver) => ({ code: `js::optionalCharacter(c, ${receiver.code}.from)`, kind: { kind: "optional", inner: { kind: "character" } } }),
  to: (receiver) => ({ code: `CharacterHandle(&c, ${receiver.code}.to->id)`, kind: { kind: "character" } }),
  who: (receiver) => number(`${receiver.code}.who`),
  via: (receiver) => ({ code: `${receiver.code}.via`, kind: { kind: "optional", inner: { kind: "skillInfo" } } }),
  fromReaction: (receiver) => boolean(`${receiver.code}.fromReaction`),
};

export const ACTION_INFO_PROPERTY_RULES = {
  type: (receiver) => string(`js::actionTypeName(${receiver.code}.type)`),
  skill: (receiver) => ({ code: `${receiver.code}.skill`, kind: { kind: "skillInfo" } }),
  targets: (receiver) => ({ code: `js::handles(c, ${receiver.code}.targets)`, kind: { kind: "array", element: { kind: "anyState" } } }),
  who: (receiver) => number(`${receiver.code}.who`),
  to: (receiver) => ({ code: `CharacterHandle(&c, ${receiver.code}.to->id)`, kind: { kind: "character" } }),
  from: (receiver) => ({ code: `CharacterHandle(&c, ${receiver.code}.from->id)`, kind: { kind: "character" } }),
  fast: (receiver) => boolean(`${receiver.code}.fast`),
};

export const AREA_PROPERTY_RULES = {
  type: (receiver) => string(`js::areaTypeName(${receiver.code})`),
  who: (receiver) => number(`${receiver.code}.who`),
  characterId: (receiver) => number(`${receiver.code}.characterId`),
  cardId: (receiver) => number(`${receiver.code}.cardId`),
};

// Array methods; callbacks are translated C++ lambdas.
export const ARRAY_RULES = {
  includes: (receiver, [item]) => boolean(`js::includes(${receiver.code}, ${item.code})`),
  filter: (receiver, [callback]) => ({ code: `js::filter(${receiver.code}, ${callback.code})`, kind: receiver.kind }),
  some: (receiver, [callback]) => boolean(`js::some(${receiver.code}, ${callback.code})`),
  every: (receiver, [callback]) => boolean(`js::every(${receiver.code}, ${callback.code})`),
  find: (receiver, [callback]) => ({ code: `js::find(${receiver.code}, ${callback.code})`, kind: { kind: "optional", inner: receiver.kind.element } }),
  map: (receiver, [callback], resultKind) => ({ code: `js::map(${receiver.code}, ${callback.code})`, kind: resultKind }),
  reduce: (receiver, [callback, initial], resultKind) =>
    initial ? { code: `js::reduce(${receiver.code}, ${callback.code}, ${initial.code})`, kind: resultKind } : null,
  slice: (receiver, [start, end]) =>
    ({ code: `js::slice(${receiver.code}, ${start ? start.code : "0.0"}${end ? `, ${end.code}` : ""})`, kind: receiver.kind }),
  // Only on a local list, as a statement (the new length JS returns is never read).
  push: (receiver, items) =>
    // TS typed the list's elements as present: an optional value (e.g. from random) is unwrapped as JS stores it.
    /^\w+$/.test(receiver.code) && items.length === 1
      ? voidResult(`${receiver.code}.push_back(${items[0].kind.kind === "optional" && receiver.kind.element?.kind !== "optional" ? `*${items[0].code}` : items[0].code})`)
      : null,
  forEach: (receiver, [callback]) => voidResult(`js::forEach(${receiver.code}, ${callback.code})`),
  toSorted: (receiver, [comparator]) => (comparator ? { code: `js::toSorted(${receiver.code}, ${comparator.code})`, kind: receiver.kind } : null),
  flatMap: (receiver, [callback], resultKind) => ({ code: `js::flatMap(${receiver.code}, ${callback.code})`, kind: resultKind }),
  at: (receiver, [index]) => ({ code: `js::at(${receiver.code}, ${index.code})`, kind: { kind: "optional", inner: receiver.kind.element } }),
};

export const ARRAY_PROPERTY_RULES = {
  length: (receiver) => number(`js::length(${receiver.code})`),
};

const ENTITY_TYPES = { support: "Support", summon: "Summon", combatStatus: "CombatStatus", status: "Status", equipment: "Equipment" };

export const EXTRA_CONTEXT_RULES = {
  playCard: ([card, target]) => {
    const rules = { first: "First", random: "Random", skipIfRequired: "SkipIfRequired" };
    if (typeof target?.literal !== "string" || !rules[target.literal]) return null;
    return voidResult(`c.playCard(js::entityHandle(c, ${card.code}), PlayCardTarget{PlayCardTarget::Rule::${rules[target.literal]}, {}})`);
  },
  selectAndPlay: ([cards, target]) => {
    if (!target) return voidResult(`c.selectAndPlay(js::ids(${cards.code}))`);
    const rules = { first: "First", random: "Random", skipIfRequired: "SkipIfRequired" };
    if (typeof target.literal === "string") {
      return rules[target.literal] ? voidResult(`c.selectAndPlay(js::ids(${cards.code}), PlayCardTarget{PlayCardTarget::Rule::${rules[target.literal]}, {}})`) : null;
    }
    if (target.kind.kind !== "array") return null;
    return voidResult(`c.selectAndPlay(js::ids(${cards.code}), js::targetList(c, ${target.code}))`);
  },
  selectAndSummon: ([summons]) => voidResult(`c.selectAndSummon(js::ids(${summons.code}))`),
  // No target: the skill's own character.
  equip: ([id, target, options]) =>
    id.kind.kind === "number" ? voidResult(`c.equip(${id.code}, ${target ? targetsOf(target) : "std::vector<AnyHandle>{AnyHandle(&c, c.selfCharacter().id(), true)}"}${options ? `, ${options.code}` : ""})`) : null,
  remainingSupportCount: ([who]) => number(`c.remainingSupportCount(${who ? who.literal === "opp" : false})`),
  allCardDefinitions: ([filter]) => {
    const kinds = { eventCard: "EventCard", support: "Support", equipment: "Equipment" };
    if (!filter) return { code: "c.allCardDefinitions()", kind: { kind: "array", element: { kind: "definition", which: "EntityDefinition" } } };
    if (filter.kind.kind === "function") {
      return { code: `c.allCardDefinitions([&](const EntityDefinition& definition) { return js::truthy((${filter.code})(&definition)); })`, kind: { kind: "array", element: { kind: "definition", which: "EntityDefinition" } } };
    }
    if (typeof filter.literal !== "string" && filter.kind.kind === "string") {
      return { code: `c.allCardDefinitions([&](const EntityDefinition& definition) { return js::cardMatches(definition, ${filter.code}); })`, kind: { kind: "array", element: { kind: "definition", which: "EntityDefinition" } } };
    }
    if (typeof filter.literal !== "string") return null;
    const test = kinds[filter.literal]
      ? `definition.type == EntityType::${kinds[filter.literal]}`
      : `definition.hasTag(intern(${JSON.stringify(filter.literal)}))`;
    return { code: `c.allCardDefinitions([](const EntityDefinition& definition) { return ${test}; })`, kind: { kind: "array", element: { kind: "definition", which: "EntityDefinition" } } };
  },
  isInInitialPile: ([card, who]) =>
    voidResultBoolean(`c.isInInitialPile(js::entityHandle(c, ${card.code})${who ? `, ${who.literal === "opp"}` : ""})`),
  hasPhaseDamage: ([scope, filter]) =>
    boolean(`c.hasPhaseDamage(${scope.literal === "all"}${filter ? `, ${filter.code}` : ""})`),
  hasPhaseReaction: ([scope, filter]) =>
    boolean(`c.hasPhaseReaction(${scope.literal === "all"}${filter ? `, ${filter.code}` : ""})`),
  createEntity: ([type, id, area, options]) => {
    if (!ENTITY_TYPES[type.literal] || options) return null;
    if (!area) return { code: `c.createEntity(EntityType::${ENTITY_TYPES[type.literal]}, ${id.code})`, kind: { kind: "optional", inner: { kind: "entity" } } };
    const areaTypes = { '"supports"': "Supports", '"summons"': "Summons", '"combatStatuses"': "CombatStatuses" };
    const fields = area.fields ?? {};
    if (!areaTypes[fields.type] || !fields.who || Object.keys(fields).length !== 2) return null;
    return { code: `c.createEntity(EntityType::${ENTITY_TYPES[type.literal]}, ${id.code}, EntityArea{.type = AreaType::${areaTypes[fields.type]}, .who = static_cast<int>(${fields.who})}) /*options*/`, kind: { kind: "optional", inner: { kind: "entity" } } };
  },
  absorbDice: ([strategy, count]) =>
    strategy.literal === "seq" || strategy.literal === "diff"
      ? { code: `js::numbers(c.absorbDice(${strategy.literal === "diff"}, ${count.code}))`, kind: { kind: "array", element: { kind: "number" } } }
      : null,
  createPileCards: ([id, count, strategy, where]) =>
    strategy.literal !== undefined
      ? voidResult(`c.createPileCards(${id.code}, ${count.code}, js::pileInsertion(${JSON.stringify(strategy.literal)})${where ? `, ${where.literal === "opp"}` : ""})`)
      : null,
  discard: (args) =>
    args.length === 1 && args[0].spread
      ? voidResult(`c.discard(${args[0].code})`)
      : args.some((argument) => argument.spread) ? null
        : voidResult(`c.discard(js::entityHandles(c, {${args.map((argument) => (argument.kind.kind === "optional" ? `*${argument.code}` : argument.code)).join(", ")}}))`),
  randomSubset: ([items, count]) => ({ code: `c.randomSubset(${items.code}, ${count.code})`, kind: items.kind }),
  selectAndCreateHandCard: ([cards]) => voidResult(`c.selectAndCreateHandCard(js::ids(${cards.code}))`),
  discardMaxCostHands: ([count, options]) => {
    const fields = options ? options.fields ?? {} : {};
    if (options && Object.keys(fields).some((key) => key !== "allowPreview")) return null;
    return { code: `c.discardMaxCostHands(${count.code}${options ? `, ${fields.allowPreview ?? "false"}) /*options*/` : ")"}`, kind: { kind: "array", element: { kind: "entity" } } };
  },
  maxCostHands: ([count, options]) => {
    const fields = options ? options.fields ?? {} : {};
    if (options && Object.keys(fields).some((key) => !["who", "filter"].includes(key))) return null;
    const filter = fields.filter ? `, ${fields.filter}` : "";
    return { code: `c.maxCostHands(${count.code}${options ? `, ${fields.who === '"opp"'}${filter}) /*options*/` : ")"}`, kind: { kind: "array", element: { kind: "entity" } } };
  },
  getExtensionState: () => ({ code: "c.getExtensionState()", kind: { kind: "json" } }),
  isSelfDisposeCausedByDefeatedHeuristically: ([filter]) =>
    boolean(`c.isSelfDisposeCausedByDefeatedHeuristically(${filter ? filter.code : ""})`),
  swapCharacterPosition: ([first, second]) => voidResult(`c.swapCharacterPosition(${targetsOf(first)}, ${targetsOf(second)})`),
  stealHandCard: ([card]) => voidResult(`c.stealHandCard(js::entityHandle(c, ${card.code}))`),
  undrawCards: ([cards, strategy, where]) =>
    strategy.literal !== undefined && cards.kind.kind === "array"
      ? voidResult(`c.undrawCards(js::entityHandles(c, ${cards.code}), js::pileInsertion(${JSON.stringify(strategy.literal)})${where ? `, ${where.literal === "opp"}` : ""})`)
      : null,
  triggerEndPhaseSkill: ([target]) => voidResult(`c.triggerEndPhaseSkill(js::entityHandle(c, ${target.code}))`),
  convertDice: ([type, count, where]) =>
    voidResult(`c.convertDice(${type.code}, ${count.literal === "all" ? -1 : count.code}${where ? `, ${where.literal === "opp"}` : ""})`),
  attachCostReduction: ([target, value]) => voidResult(`c.attachCostReduction(${targetsOf(target)}${value ? `, ${value.code}` : ""})`),
  attachCostIncrease: ([target, value]) => voidResult(`c.attachCostIncrease(${targetsOf(target)}${value ? `, ${value.code}` : ""})`),
  attach: ([id, target, options]) => voidResult(`c.attach(${id.code}, js::entityHandle(c, ${target.code})${options ? `, ${options.code}` : ""})`),
  emitCustomEvent: ([event, argument]) =>
    event.kind.kind === "customEvent" ? voidResult(`c.emitCustomEvent(${event.code}${argument ? `, js::toJson(${argument.code})` : ""})`) : null,
  handleCustomEventInline: ([event, argument]) =>
    event.kind.kind === "customEvent" ? voidResult(`c.handleCustomEventInline(${event.code}${argument ? `, js::toJson(${argument.code})` : ""})`) : null,
  cleanAura: ([what, target]) => voidResult(`c.cleanAura(${what.literal === "all" ? -1 : what.code}, ${targetsOf(target)})`),
};
