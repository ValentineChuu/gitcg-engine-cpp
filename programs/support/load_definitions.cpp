// Loads dump_definitions.mjs, retaining registration, variable, skill, and cost Map order.
#include "programs/support/load_definitions.h"

#include <stdexcept>
#include "programs/support/json_io.h"

namespace gitcg::testing {
namespace {
template <typename T> T enumValue(const std::string& text, std::initializer_list<std::pair<const char*, T>> choices) {
  for (const auto& [name, value] : choices) if (text == name) return value;
  throw std::runtime_error("Unknown definition enum " + text);
}
bool boolean(const JsonValue& value, std::string_view key, bool fallback = false) {
  auto* field = value.find(key);
  return field && !field->isNull() ? field->asBool() : fallback;
}
VersionInfo version(const JsonValue& value) {
  const auto& source = value.at("version");
  return {source.at("predicate").asString() == "since", source.at("version").asString()};
}
std::vector<Name> tags(const JsonValue& value) {
  std::vector<Name> result;
  for (const auto& tag : value.at("tags").asArray()) result.push_back(intern(tag.asString()));
  return result;
}
std::vector<std::pair<Name, VariableConfig>> variables(const JsonValue& value) {
  std::vector<std::pair<Name, VariableConfig>> result;
  for (const auto& source : value.at("varConfigs").asArray()) {
    VariableConfig config;
    config.initialValue = source.at("initialValue").asNumber();
    config.lowerBound = source.at("lowerBound").asNumber();
    config.upperBound = source.at("upperBound").asNumber();
    const auto& recreate = source.at("recreateBehavior");
    config.recreateBehavior = enumValue<RecreateBehavior>(recreate.at("type").asString(), {
        {"default", RecreateBehavior::Default}, {"overwrite", RecreateBehavior::Overwrite},
        {"keep", RecreateBehavior::Keep}, {"takeMax", RecreateBehavior::TakeMax}, {"append", RecreateBehavior::Append}});
    if (config.recreateBehavior == RecreateBehavior::Append) {
      config.appendLimit = recreate.at("appendLimit").asNumber();
      config.appendValue = recreate.at("appendValue").asNumber();
    }
    result.emplace_back(intern(source.at("name").asString()), config);
  }
  return result;
}
std::vector<const SkillDefinition*> skills(const JsonValue& value, LoadedDefinitions& owner) {
  std::vector<const SkillDefinition*> result;
  for (const auto& source : value.at("skills").asArray()) {
    auto skill = std::make_unique<SkillDefinition>();
    skill->id = source.at("id").asNumber();
    skill->ownerType = enumValue<OwnerType>(source.at("ownerType").asString(), {
        {"character", OwnerType::Character}, {"eventCard", OwnerType::EventCard}, {"status", OwnerType::Status},
        {"combatStatus", OwnerType::CombatStatus}, {"equipment", OwnerType::Equipment}, {"support", OwnerType::Support},
        {"summon", OwnerType::Summon}, {"attachment", OwnerType::Attachment}, {"extension", OwnerType::Extension}});
    const auto& type = source.at("skillType");
    if (!type.isNull()) skill->skillType = enumValue<SkillType>(type.asString(), {
        {"normal", SkillType::Normal}, {"elemental", SkillType::Elemental}, {"burst", SkillType::Burst},
        {"technique", SkillType::Technique}, {"playCard", SkillType::PlayCard}});
    bool found = false;
    for (int i = 0; i <= static_cast<int>(EventName::RequestTriggerEndPhaseSkill); ++i) {
      if (source.at("triggerOn").asString() == eventNameText(static_cast<EventName>(i))) {
        skill->triggerOn = static_cast<EventName>(i); found = true; break;
      }
    }
    if (!found) throw std::runtime_error("Unknown skill trigger");
    auto* usage = source.find("usagePerRoundVariableName");
    if (usage && !usage->isNull()) skill->usagePerRoundVariableName = intern(usage->asString());
    const auto& config = source.at("initiativeSkillConfig");
    if (!config.isNull()) {
      InitiativeSkillConfig initiative;
      for (const auto& cost : config.at("requiredCost").asArray()) {
        initiative.requiredCost.set(cost.asArray()[0].asInt(), cost.asArray()[1].asInt());
      }
      initiative.costSize = config.at("costSize").asInt();
      initiative.diceCostSize = config.at("diceCostSize").asInt();
      initiative.gainEnergy = boolean(config, "gainEnergy", true);
      initiative.shouldFast = boolean(config, "shouldFast");
      initiative.alwaysCharged = boolean(config, "alwaysCharged");
      initiative.alwaysPlunging = boolean(config, "alwaysPlunging");
      initiative.hidden = boolean(config, "hidden");
      initiative.omitEvents = boolean(config, "omitEvents");
      skill->initiativeSkillConfig = std::move(initiative);
    }
    result.push_back(skill.get());
    owner.skills.push_back(std::move(skill));
  }
  return result;
}
void entities(const JsonValue& values, bool attachment, LoadedDefinitions& owner) {
  for (const auto& source : values.asArray()) {
    auto entity = std::make_unique<EntityDefinition>();
    entity->id = source.at("id").asInt();
    entity->type = enumValue<EntityType>(source.at("type").asString(), {
        {"status", EntityType::Status}, {"combatStatus", EntityType::CombatStatus}, {"summon", EntityType::Summon},
        {"support", EntityType::Support}, {"equipment", EntityType::Equipment},
        {"eventCard", EntityType::EventCard}, {"attachment", EntityType::Attachment}});
    entity->version = version(source);
    entity->obtainable = boolean(source, "obtainable", true);
    auto* visible = source.find("visibleVarName");
    if (visible && !visible->isNull()) entity->visibleVarName = intern(visible->asString());
    entity->tags = tags(source);
    if (!source.at("hintText").isNull()) entity->hintText = source.at("hintText").asString();
    entity->disableTuning = boolean(source, "disableTuning");
    entity->varConfigs = variables(source);
    entity->disposeWhenUsageIsZero = boolean(source, "disposeWhenUsageIsZero");
    entity->disposeOnMasterDefeated = boolean(source, "disposeOnMasterDefeated");
    entity->skills = skills(source, owner);
    for (const auto& key : source.at("descriptionKeys").asArray()) {
      entity->descriptionDictionary.emplace_back(key.asString(), DescriptionEntry{});
    }
    if (attachment) { owner.data.attachments.add(entity.get()); owner.attachments.push_back(std::move(entity)); }
    else { owner.data.entities.add(entity.get()); owner.entities.push_back(std::move(entity)); }
  }
}
}  // namespace
LoadedDefinitions loadDefinitions(const JsonValue& dump) {
  LoadedDefinitions result;
  result.source = dump;
  entities(dump.at("entities"), false, result);
  entities(dump.at("attachments"), true, result);
  for (const auto& source : dump.at("characters").asArray()) {
    auto character = std::make_unique<CharacterDefinition>();
    character->id = source.at("id").asInt();
    character->version = version(source);
    character->tags = tags(source);
    character->varConfigs = variables(source);
    character->skills = skills(source, result);
    if (!source.at("associatedNightsoulsBlessing").isNull()) {
      character->associatedNightsoulsBlessing = result.data.entities.find(source.at("associatedNightsoulsBlessing").asInt());
      if (!character->associatedNightsoulsBlessing) throw std::runtime_error("Missing nightsoul definition");
    }
    for (const auto& reaction : source.at("enabledLunarReactions").asArray()) character->enabledLunarReactions.push_back(reaction.asInt());
    auto* special = source.find("specialEnergy");
    if (special && !special->isNull()) character->specialEnergy = SpecialEnergyConfig{
        intern(special->at("variableName").asString()), special->at("slotSize").asInt()};
    result.data.characters.add(character.get());
    result.characters.push_back(std::move(character));
  }
  for (const auto& source : dump.at("extensions").asArray()) {
    auto extension = std::make_unique<ExtensionDefinition>();
    extension->id = source.at("id").asInt();
    extension->version = version(source);
    extension->initialState = source.at("initialState");
    extension->skills = skills(source, result);
    result.data.extensions.add(extension.get());
    result.extensions.push_back(std::move(extension));
  }
  return result;
}
LoadedDefinitions loadDefinitions(const std::string& path) { return loadDefinitions(readJson(path)); }
}  // namespace gitcg::testing
