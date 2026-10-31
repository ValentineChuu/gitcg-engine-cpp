// The C++ kind of a TypeScript value in card code, read from the checker type: what the translated expression
// is in C++ (a double, a live character handle, an event argument, an optional of something, ...).

import { ts } from "../typed_program.mjs";

const CHARACTER_TYPES = new Set(["Character", "ReadonlyCharacter", "CharacterBase", "TypedCharacter"]);
const ENTITY_TYPES = new Set([
  "Entity", "ReadonlyEntity", "EntityWithoutMaster", "ReadonlyEntityWithoutMaster", "TypedEntity", "Attachment", "ReadonlyAttachment",
  "TypedAttachment", "PlainEntityState",
]);
const EVENT_ARGUMENT_CLASSES = {
  EventArg: "EventArg",
  PlayerEventArg: "PlayerEventArg",
  ModifyRollEventArg: "ModifyRollEventArg",
  ActionEventArg: "ActionEventArg",
  ModifyAction0EventArg: "ModifyActionEventArg",
  ModifyAction1EventArg: "ModifyActionEventArg",
  ModifyAction2EventArg: "ModifyActionEventArg",
  ModifyAction3EventArg: "ModifyActionEventArg",
  ModifyAction4EventArg: "ModifyActionEventArg",
  ModifyActionEventArgBase: "ModifyActionEventArg",
  SwitchActiveEventArg: "SwitchActiveEventArg",
  UseSkillEventArg: "UseSkillEventArg",
  PlayCardEventArg: "PlayCardEventArg",
  DamageOrHealEventArg: "DamageOrHealEventArg",
  ModifyDamage0EventArg: "ModifyDamageEventArg",
  ModifyDamage1EventArg: "ModifyDamageEventArg",
  ModifyDamage2EventArg: "ModifyDamageEventArg",
  ModifyDamage3EventArg: "ModifyDamageEventArg",
  ModifyDamageEventArgBase: "ModifyDamageEventArg",
  ModifyHeal0EventArg: "ModifyHealEventArg",
  ModifyHeal1EventArg: "ModifyHealEventArg",
  ZeroHealthEventArg: "ZeroHealthEventArg",
  EntityEventArg: "EntityEventArg",
  EnterEventArg: "EnterEventArg",
  DisposeEventArg: "DisposeEventArg",
  CharacterEventArg: "CharacterEventArg",
  ReactionEventArg: "ReactionEventArg",
  ModifyReactionEventArg: "ModifyReactionEventArg",
  TransformDefinitionEventArg: "TransformDefinitionEventArg",
  HandCardInsertedEventArg: "HandCardInsertedEventArg",
  GenerateDiceEventArg: "GenerateDiceEventArg",
  VariableEventArg: "VariableEventArg",
  BeforeVariableEventArg: "BeforeVariableEventArg",
  SelectCardEventArg: "SelectCardEventArg",
  CustomEventEventArg: "CustomEventEventArg",
  StrictInitiativeSkillEventArg: "InitiativeSkillArgument",
};

export function symbolNameOf(type) {
  return (type.aliasSymbol ?? type.getSymbol())?.getName() ?? null;
}

// { kind, ... } where kind is one of: number, boolean, string, void, context, character, entity, anyState,
// eventArgument (cppClass), optional (inner), array (element), definition, query, object, unknown.
export function valueKind(checker, type) {
  if (type.flags & ts.TypeFlags.Void || type.flags & ts.TypeFlags.Undefined) return { kind: "void" };
  if (type.isUnion()) {
    const present = type.types.filter((member) => !(member.flags & (ts.TypeFlags.Undefined | ts.TypeFlags.Null)));
    if (present.length !== type.types.length) {
      return { kind: "optional", inner: present.length === 1 ? valueKind(checker, present[0]) : unionKind(checker, present) };
    }
    return unionKind(checker, type.types);
  }
  if (type.flags & (ts.TypeFlags.NumberLike | ts.TypeFlags.EnumLike)) return { kind: "number" };
  // Definition handles are branded numbers (number & { _meta: ... }).
  if (type.isIntersection() && type.types.some((member) => member.flags & ts.TypeFlags.NumberLike)) return { kind: "number" };
  if (type.flags & ts.TypeFlags.BooleanLike) return { kind: "boolean" };
  if (type.flags & ts.TypeFlags.StringLike) return { kind: "string" };
  const name = symbolNameOf(type);
  if (name === "Omit" || name === "SkillContext" || name === "TypedSkillContext") return { kind: "context" };
  if (CHARACTER_TYPES.has(name)) return { kind: "character" };
  if (ENTITY_TYPES.has(name)) return { kind: "entity" };
  if (name === "RxEntityState") {
    // The alias hides the concrete class: look at the type itself (or its union members).
    const concreteName = type.getSymbol()?.getName();
    if (CHARACTER_TYPES.has(concreteName)) return { kind: "character" };
    if (ENTITY_TYPES.has(concreteName)) return { kind: "entity" };
    if (type.isUnion()) return unionKind(checker, type.types);
    return { kind: "anyState" };
  }
  if (name && EVENT_ARGUMENT_CLASSES[name]) return { kind: "eventArgument", cppClass: EVENT_ARGUMENT_CLASSES[name] };
  if (name === "Array" || name === "ReadonlyArray" || checker.isArrayType?.(type) || checker.isTupleType?.(type)) {
    const [element] = checker.getTypeArguments(type);
    return { kind: "array", element: element ? valueKind(checker, element) : { kind: "unknown" } };
  }
  if (name === "SkillInfo" || name === "InitiativeSkillInfo" || name === "PlayCardSkillInfo") return { kind: "skillInfo" };
  if (name === "SwitchActiveInfo") return { kind: "switchInfo" };
  if (name === "EntityArea") return { kind: "area" };
  if (name === "WithActionDetail" || name === "ActionInfo") return { kind: "actionInfo" };
  if (name === "EntityDefinition" || name === "CharacterDefinition" || name === "SkillDefinition" || name === "InitiativeSkillDefinition") {
    return { kind: "definition", which: name };
  }
  if (name === "PrimaryQuery" || name === "CompositeQuery" || name === "Assign" || name === "IDollar") return { kind: "query" };
  return { kind: "unknown", name: name ?? checker.typeToString(type).slice(0, 60) };
}

function unionKind(checker, types) {
  const kinds = types.map((member) => valueKind(checker, member));
  if (kinds.every((kind) => kind.kind === kinds[0].kind)) return kinds[0];
  if (kinds.every((kind) => kind.kind === "boolean" || kind.kind === "number")) return { kind: "number" };
  if (kinds.every((kind) => ["character", "entity", "anyState"].includes(kind.kind))) return { kind: "anyState" };
  return { kind: "unknown", name: kinds.map((kind) => kind.kind).join("|") };
}
