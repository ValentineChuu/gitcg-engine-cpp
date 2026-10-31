// Serialize the same fields and ordered collections as the Node reference dump.
#include "definitions/registry/dump.h"
#include <stdexcept>
namespace gitcg {
namespace {
JsonValue version(const VersionInfo& value) { return JsonValue::Object{{"predicate", value.isSince ? "since" : "until"}, {"version", value.version}}; }
JsonValue names(const std::vector<Name>& values) { auto result = JsonValue::array(); for (Name name : values) result.push(nameText(name)); return result; }
const char* ownerText(OwnerType value) {
  switch (value) {
    case OwnerType::Character: return "character";
    case OwnerType::EventCard: return "eventCard";
    case OwnerType::Status: return "status";
    case OwnerType::CombatStatus: return "combatStatus";
    case OwnerType::Equipment: return "equipment";
    case OwnerType::Support: return "support";
    case OwnerType::Summon: return "summon";
    case OwnerType::Attachment: return "attachment";
    case OwnerType::Extension: return "extension";
  }
  throw std::runtime_error("Unknown owner type");
}
JsonValue typeText(SkillType value) {
  switch (value) {
    case SkillType::None: return nullptr;
    case SkillType::Normal: return "normal";
    case SkillType::Elemental: return "elemental";
    case SkillType::Burst: return "burst";
    case SkillType::Technique: return "technique";
    case SkillType::PlayCard: return "playCard";
  }
  throw std::runtime_error("Unknown skill type");
}
JsonValue variables(const std::vector<std::pair<Name, VariableConfig>>& configs, bool character = false) {
  auto result = JsonValue::array();
  size_t index = 0;
  for (const auto& [name, config] : configs) {
    auto recreate = JsonValue::object();
    switch (config.recreateBehavior) {
      case RecreateBehavior::Default: recreate["type"] = "default"; break;
      case RecreateBehavior::Overwrite: recreate["type"] = "overwrite"; break;
      case RecreateBehavior::Keep: recreate["type"] = "keep"; break;
      case RecreateBehavior::TakeMax: recreate["type"] = "takeMax"; break;
      case RecreateBehavior::Append: recreate["type"] = "append"; recreate["appendLimit"] = config.appendLimit; recreate["appendValue"] = config.appendValue; break;
    }
    if (character && index < 6) result.push(JsonValue::Object{{"name", nameText(name)}, {"initialValue", config.initialValue}, {"lowerBound", config.lowerBound}, {"upperBound", config.upperBound}, {"recreateBehavior", recreate}});
    else result.push(JsonValue::Object{{"name", nameText(name)}, {"initialValue", config.initialValue}, {"recreateBehavior", recreate}, {"lowerBound", config.lowerBound}, {"upperBound", config.upperBound}});
    ++index;
  }
  return result;
}
JsonValue skills(const std::vector<const SkillDefinition*>& values) {
  auto result = JsonValue::array();
  for (const auto* skill : values) {
    JsonValue config;
    if (skill->initiativeSkillConfig) {
      const auto& value = *skill->initiativeSkillConfig;
      auto cost = JsonValue::array();
      for (const auto& [type, count] : value.requiredCost.entries()) cost.push(JsonValue::Array{type, count});
      config = JsonValue::Object{{"requiredCost", cost}, {"costSize", value.costSize}, {"diceCostSize", value.diceCostSize}, {"gainEnergy", value.gainEnergy}, {"shouldFast", value.shouldFast}, {"alwaysCharged", value.alwaysCharged}, {"alwaysPlunging", value.alwaysPlunging}, {"hidden", value.hidden}, {"omitEvents", value.omitEvents}};
    }
    result.push(JsonValue::Object{{"id", skill->id}, {"ownerType", ownerText(skill->ownerType)}, {"skillType", typeText(skill->skillType)}, {"triggerOn", eventNameText(skill->triggerOn)}, {"usagePerRoundVariableName", skill->usagePerRoundVariableName ? JsonValue(nameText(*skill->usagePerRoundVariableName)) : JsonValue()}, {"initiativeSkillConfig", config}});
  }
  return result;
}
const char* entityTypeText(EntityType type) {
  switch (type) {
    case EntityType::EventCard: return "eventCard";
    case EntityType::Status: return "status";
    case EntityType::CombatStatus: return "combatStatus";
    case EntityType::Equipment: return "equipment";
    case EntityType::Support: return "support";
    case EntityType::Summon: return "summon";
    case EntityType::Attachment: return "attachment";
  }
  throw std::runtime_error("Unknown entity type");
}
JsonValue entity(const EntityDefinition& value) {
  bool attachment = value.type == EntityType::Attachment;
  auto descriptionKeys = JsonValue::array();
  for (const auto& [key, getter] : value.descriptionDictionary) descriptionKeys.push(key);
  return JsonValue::Object{{"type", entityTypeText(value.type)}, {"id", value.id}, {"version", version(value.version)}, {"obtainable", attachment ? JsonValue() : JsonValue(value.obtainable)}, {"visibleVarName", value.visibleVarName ? JsonValue(nameText(*value.visibleVarName)) : JsonValue()}, {"tags", names(value.tags)}, {"hintText", value.hintText ? JsonValue(*value.hintText) : JsonValue()}, {"disableTuning", attachment ? JsonValue() : JsonValue(value.disableTuning)}, {"varConfigs", variables(value.varConfigs)}, {"disposeWhenUsageIsZero", attachment ? JsonValue() : JsonValue(value.disposeWhenUsageIsZero)}, {"disposeOnMasterDefeated", attachment ? JsonValue() : JsonValue(value.disposeOnMasterDefeated)}, {"skills", skills(value.skills)}, {"descriptionKeys", descriptionKeys}};
}
}  // namespace
JsonValue dumpDefinitions(const GameData& data) {
  JsonValue result = JsonValue::object();
  result["characters"] = JsonValue::array();
  for (const auto* value : data.characters.values()) {
    auto reactions = JsonValue::array(); for (int reaction : value->enabledLunarReactions) reactions.push(reaction);
    JsonValue specialEnergy;
    if (value->specialEnergy) specialEnergy = JsonValue::Object{{"variableName", nameText(value->specialEnergy->variableName)}, {"slotSize", value->specialEnergy->slotSize}};
    result["characters"].push(JsonValue::Object{{"id", value->id}, {"version", version(value->version)}, {"tags", names(value->tags)}, {"varConfigs", variables(value->varConfigs, true)}, {"skills", skills(value->skills)}, {"associatedNightsoulsBlessing", value->associatedNightsoulsBlessing ? JsonValue(value->associatedNightsoulsBlessing->id) : JsonValue()}, {"enabledLunarReactions", reactions}, {"specialEnergy", specialEnergy}});
  }
  result["entities"] = JsonValue::array(); for (const auto* value : data.entities.values()) result["entities"].push(entity(*value));
  result["attachments"] = JsonValue::array(); for (const auto* value : data.attachments.values()) result["attachments"].push(entity(*value));
  result["extensions"] = JsonValue::array(); for (const auto* value : data.extensions.values()) result["extensions"].push(JsonValue::Object{{"id", value->id}, {"version", version(value->version)}, {"initialState", value->initialState}, {"skills", skills(value->skills)}});
  return result;
}
}  // namespace gitcg
