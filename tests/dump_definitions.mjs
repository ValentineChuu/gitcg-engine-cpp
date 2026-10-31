// Dumps every resolved definition of @gi-tcg/data (what the engine plays with) as JSON, in the data's Map order.
// The C++ build prints the same dump (programs/dump_definitions.cpp); compare_definitions.mjs diffs the two.
// Usage: node tests/dump_definitions.mjs > tests/generated/definitions_node.json

import getData from "@gi-tcg/data";

const data = getData();

function versionOf(definition) {
  const value = definition.version?.value ?? {};
  return { predicate: value.predicate ?? null, version: value.version ?? null };
}

function varConfigsOf(varConfigs) {
  return Object.entries(varConfigs).map(([name, config]) => ({ name, ...config }));
}

function skillOf(skill) {
  const config = skill.initiativeSkillConfig;
  return {
    id: skill.id,
    ownerType: skill.ownerType,
    skillType: skill.skillType,
    triggerOn: skill.triggerOn,
    usagePerRoundVariableName: skill.usagePerRoundVariableName,
    initiativeSkillConfig: config
      ? {
          requiredCost: [...config.requiredCost.entries()],
          costSize: config.computed$costSize,
          diceCostSize: config.computed$diceCostSize,
          gainEnergy: config.gainEnergy,
          shouldFast: config.shouldFast,
          alwaysCharged: config.alwaysCharged,
          alwaysPlunging: config.alwaysPlunging,
          hidden: config.hidden,
          omitEvents: config.omitEvents,
        }
      : null,
  };
}

function entityOf(entity) {
  return {
    type: entity.type,
    id: entity.id,
    version: versionOf(entity),
    obtainable: entity.obtainable ?? null,
    visibleVarName: entity.visibleVarName,
    tags: entity.tags,
    hintText: entity.hintText ?? null,
    disableTuning: entity.disableTuning ?? null,
    varConfigs: varConfigsOf(entity.varConfigs),
    disposeWhenUsageIsZero: entity.disposeWhenUsageIsZero ?? null,
    disposeOnMasterDefeated: entity.disposeOnMasterDefeated ?? null,
    skills: entity.skills.map(skillOf),
    descriptionKeys: Object.keys(entity.descriptionDictionary ?? {}),
  };
}

function characterOf(character) {
  return {
    id: character.id,
    version: versionOf(character),
    tags: character.tags,
    varConfigs: varConfigsOf(character.varConfigs),
    skills: character.skills.map(skillOf),
    associatedNightsoulsBlessing: character.associatedNightsoulsBlessing?.id ?? null,
    enabledLunarReactions: character.enabledLunarReactions,
    specialEnergy: character.specialEnergy,
  };
}

function extensionOf(extension) {
  return {
    id: extension.id,
    version: versionOf(extension),
    initialState: extension.initialState,
    skills: extension.skills.map(skillOf),
  };
}

const dump = {
  characters: [...data.characters.values()].map(characterOf),
  entities: [...data.entities.values()].map(entityOf),
  attachments: [...data.attachments.values()].map(entityOf),
  extensions: [...data.extensions.values()].map(extensionOf),
};
process.stdout.write(JSON.stringify(dump) + "\n");
