// Export Piovium's card/character catalog (structural data only; behavior is code) to <data>/derived/piovium_catalog.json (data_paths.mjs),
// and copy its deck share-code table (share id -> definition id) to <data>/derived/piovium_share_ids.json.
// Usage: node tools/export_catalog.mjs

import { copyFileSync, mkdirSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import getData from "@gi-tcg/data";
import { dataFile } from "../data_paths.mjs";

const DECK_CARD_TYPES = new Set(["eventCard", "equipment", "support"]);
const data = getData();

function initialVariables(varConfigs) {
  return Object.fromEntries(Object.entries(varConfigs ?? {}).map(([name, config]) => [name, config.initialValue]));
}

function sinceVersion(version) {
  return version?.value?.predicate === "since" ? version.value.version : null;
}

function initiativeSkills(skills) {
  return (skills ?? [])
    .filter((skill) => skill.initiativeSkillConfig)
    .map((skill) => ({
      id: skill.id,
      skillType: skill.skillType,
      cost: Object.fromEntries(skill.initiativeSkillConfig.requiredCost ?? []),
      gainEnergy: skill.initiativeSkillConfig.gainEnergy,
      shouldFast: skill.initiativeSkillConfig.shouldFast,
    }));
}

function triggers(skills) {
  return [...new Set((skills ?? []).map((skill) => skill.triggerOn).filter((name) => name && name !== "initiative"))];
}

const characters = [...data.characters.values()].map((character) => ({
  id: character.id,
  tags: character.tags,
  variables: initialVariables(character.varConfigs),
  skillIds: character.skillIds,
  skills: initiativeSkills(character.skills),
  passiveTriggers: triggers(character.skills),
  specialEnergy: character.specialEnergy ?? null,
  since: sinceVersion(character.version),
}));

const entities = [...data.entities.values()].map((entity) => ({
  id: entity.id,
  type: entity.type,
  isDeckCard: DECK_CARD_TYPES.has(entity.type),
  obtainable: entity.obtainable ?? null,
  tags: entity.tags,
  disableTuning: entity.disableTuning ?? null,
  variables: initialVariables(entity.varConfigs),
  visibleVarName: entity.visibleVarName ?? null,
  playCost: initiativeSkills(entity.skills).find((skill) => skill.skillType === "playCard")?.cost ?? null,
  skills: initiativeSkills(entity.skills).filter((skill) => skill.skillType !== "playCard"),
  triggers: triggers(entity.skills),
  since: sinceVersion(entity.version),
}));

// Attachments are markers on hand cards (e.g. Electric Shock, cost changes, Untunable).
const attachments = [...data.attachments.values()].map((attachment) => ({
  id: attachment.id,
  variables: initialVariables(attachment.varConfigs),
}));

const outputDirectory = dataFile("derived");
mkdirSync(outputDirectory, { recursive: true });
writeFileSync(join(outputDirectory, "piovium_catalog.json"), JSON.stringify({ characters, entities, attachments }, null, 1));
copyFileSync(new URL("../piovium/packages/assets-manager/src/data/share_id.json", import.meta.url), join(outputDirectory, "piovium_share_ids.json"));
console.log(`exported ${characters.length} characters, ${entities.length} entities, ${attachments.length} attachments`);
