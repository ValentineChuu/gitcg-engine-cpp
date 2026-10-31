// Re-serializes represented fields independently; source-only null distinctions are normalized explicitly.
#include "programs/support/verify_definitions.h"

#include <stdexcept>

namespace gitcg::testing {
namespace {
void require(bool condition, const std::string& field) { if (!condition) throw std::runtime_error("Definition loader mismatch: " + field); }
template <typename T> void checkVersion(const T& definition, const JsonValue& source) {
  require(definition.version.isSince == (source.at("version").at("predicate").asString() == "since"), "version predicate");
  require(definition.version.version == source.at("version").at("version").asString(), "version");
}
void checkSkills(const std::vector<const SkillDefinition*>& skills, const JsonValue& source) {
  const auto& values = source.at("skills").asArray();
  require(skills.size() == values.size(), "skills count");
  const std::vector<std::string> owners{"character", "eventCard", "status", "combatStatus", "equipment", "support", "summon", "attachment", "extension"};
  const std::vector<std::string> types{"", "normal", "elemental", "burst", "technique", "playCard"};
  for (size_t i = 0; i < skills.size(); ++i) {
    const auto& skill = *skills[i]; const auto& value = values[i];
    require(skill.id == value.at("id").asNumber(), "skill id/order");
    require(owners.at(static_cast<int>(skill.ownerType)) == value.at("ownerType").asString(), "skill owner");
    require(types.at(static_cast<int>(skill.skillType)) == (value.at("skillType").isNull() ? "" : value.at("skillType").asString()), "skill type");
    require(eventNameText(skill.triggerOn) == value.at("triggerOn").asString(), "skill trigger");
    auto* usage = value.find("usagePerRoundVariableName");
    require(skill.usagePerRoundVariableName.has_value() == (usage && !usage->isNull()), "usage variable existence");
    if (skill.usagePerRoundVariableName) require(nameText(*skill.usagePerRoundVariableName) == usage->asString(), "usage variable");
    require(!skill.action && !skill.filter, "static skills have empty functions");
    const auto& config = value.at("initiativeSkillConfig");
    require(skill.initiativeSkillConfig.has_value() == !config.isNull(), "initiative config existence");
    if (!skill.initiativeSkillConfig) continue;
    const auto& actual = *skill.initiativeSkillConfig;
    require(!actual.getTarget, "static targets have no function");
    const auto& costs = config.at("requiredCost").asArray();
    require(actual.requiredCost.entries().size() == costs.size(), "cost count");
    for (size_t k = 0; k < costs.size(); ++k) require(actual.requiredCost.entries()[k].first == costs[k].asArray()[0].asInt()
        && actual.requiredCost.entries()[k].second == costs[k].asArray()[1].asInt(), "cost entries/order");
    require(actual.costSize == config.at("costSize").asInt() && actual.diceCostSize == config.at("diceCostSize").asInt(), "cost sizes");
#define FLAG(field) require(actual.field == config.at(#field).asBool(), "initiative " #field);
    FLAG(gainEnergy) FLAG(shouldFast) FLAG(alwaysCharged) FLAG(alwaysPlunging) FLAG(hidden) FLAG(omitEvents)
#undef FLAG
  }
}
template <typename T> void checkVariables(const T& definition, const JsonValue& source) {
  const auto& values = source.at("varConfigs").asArray();
  require(definition.varConfigs.size() == values.size(), "variable count");
  const std::vector<std::string> recreateNames{"default", "overwrite", "keep", "takeMax", "append"};
  for (size_t i = 0; i < values.size(); ++i) {
    const auto& [name, config] = definition.varConfigs[i]; const auto& value = values[i];
    require(nameText(name) == value.at("name").asString(), "variable name/order");
    require(config.initialValue == value.at("initialValue").asNumber() && config.lowerBound == value.at("lowerBound").asNumber()
        && config.upperBound == value.at("upperBound").asNumber(), "variable values/bounds");
    require(recreateNames.at(static_cast<int>(config.recreateBehavior)) == value.at("recreateBehavior").at("type").asString(), "recreate type");
    if (config.recreateBehavior == RecreateBehavior::Append) require(config.appendLimit == value.at("recreateBehavior").at("appendLimit").asNumber()
        && config.appendValue == value.at("recreateBehavior").at("appendValue").asNumber(), "append values");
  }
  const auto& tags = source.at("tags").asArray();
  require(tags.size() == definition.tags.size(), "tag count");
  for (size_t i = 0; i < tags.size(); ++i) require(nameText(definition.tags[i]) == tags[i].asString(), "tag/order");
}
bool sourceBool(const JsonValue& value, const std::string& key, bool fallback = false) {
  return value.at(key).isNull() ? fallback : value.at(key).asBool();
}
}  // namespace
void verifyDefinitions(const LoadedDefinitions& loaded) {
  for (const auto* key : {"entities", "attachments"}) {
    const auto& actual = std::string(key) == "entities" ? loaded.data.entities.values() : loaded.data.attachments.values();
    const auto& expected = loaded.source.at(key).asArray();
    require(actual.size() == expected.size(), std::string(key) + " count");
    const std::vector<std::string> types{"eventCard", "status", "combatStatus", "equipment", "support", "summon", "attachment"};
    for (size_t i = 0; i < actual.size(); ++i) {
      const auto& def = *actual[i]; const auto& value = expected[i];
      require(def.id == value.at("id").asInt(), "entity registration order");
      require(types.at(static_cast<int>(def.type)) == value.at("type").asString(), "entity type");
      checkVersion(def, value); checkVariables(def, value); checkSkills(def.skills, value);
      require(def.obtainable == sourceBool(value, "obtainable", true) && def.disableTuning == sourceBool(value, "disableTuning")
          && def.disposeWhenUsageIsZero == sourceBool(value, "disposeWhenUsageIsZero")
          && def.disposeOnMasterDefeated == sourceBool(value, "disposeOnMasterDefeated"), "entity flags");
      auto* visible = value.find("visibleVarName");
      require(def.visibleVarName.has_value() == (visible && !visible->isNull()), "visible variable existence");
      if (def.visibleVarName) require(nameText(*def.visibleVarName) == visible->asString(), "visible variable");
      require(def.hintText.has_value() == !value.at("hintText").isNull(), "hint existence");
      if (def.hintText) require(*def.hintText == value.at("hintText").asString(), "hint");
      const auto& descriptions = value.at("descriptionKeys").asArray();
      require(def.descriptionDictionary.size() == descriptions.size(), "description key count");
      for (size_t k = 0; k < descriptions.size(); ++k) require(def.descriptionDictionary[k].first == descriptions[k].asString()
          && !def.descriptionDictionary[k].second, "description key/order and empty function");
      require(!def.modifications, "static attachment modifications are absent");
    }
  }
  const auto& characters = loaded.source.at("characters").asArray();
  require(characters.size() == loaded.data.characters.values().size(), "character count");
  for (size_t i = 0; i < characters.size(); ++i) {
    const auto& value = characters[i]; const auto& def = *loaded.data.characters.values()[i];
    require(def.id == value.at("id").asInt(), "character registration order");
    checkVersion(def, value); checkVariables(def, value); checkSkills(def.skills, value);
    require(bool(def.associatedNightsoulsBlessing) == !value.at("associatedNightsoulsBlessing").isNull(), "nightsoul existence");
    if (def.associatedNightsoulsBlessing) require(def.associatedNightsoulsBlessing->id == value.at("associatedNightsoulsBlessing").asInt(), "nightsoul id");
    require(def.enabledLunarReactions.size() == value.at("enabledLunarReactions").asArray().size(), "lunar reaction count");
    for (size_t k = 0; k < def.enabledLunarReactions.size(); ++k) require(def.enabledLunarReactions[k] == value.at("enabledLunarReactions").asArray()[k].asInt(), "lunar reaction/order");
    auto* special = value.find("specialEnergy");
    require(def.specialEnergy.has_value() == (special && !special->isNull()), "special energy existence");
    if (def.specialEnergy) require(nameText(def.specialEnergy->variableName) == special->at("variableName").asString()
        && def.specialEnergy->slotSize == special->at("slotSize").asInt(), "special energy fields");
  }
  const auto& extensions = loaded.source.at("extensions").asArray();
  require(extensions.size() == loaded.data.extensions.values().size(), "extension count");
  for (size_t i = 0; i < extensions.size(); ++i) {
    const auto& value = extensions[i]; const auto& def = *loaded.data.extensions.values()[i];
    require(def.id == value.at("id").asInt(), "extension registration order");
    checkVersion(def, value); checkSkills(def.skills, value);
    require(def.initialState == value.at("initialState"), "extension initial state");
  }
}
}  // namespace gitcg::testing
