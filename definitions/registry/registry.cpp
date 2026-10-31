// The registry retains all versions; first id registration fixes Map order independently of chosen version.
#include "definitions/registry/registry.h"
#include "definitions/registry/storage.h"
#include "definitions/skills/skill_model.h"
#include "core/json/json_text.h"
#include <stdexcept>
#include <unordered_set>

namespace gitcg {
DefinitionRegistry::DefinitionRegistry() : storage_(std::make_unique<DefinitionStorage>()) {}
DefinitionRegistry::~DefinitionRegistry() = default;
std::shared_ptr<LambdaSlots> DefinitionRegistry::lambdaSlots() const { return storage_->slots; }
const CustomEvent* DefinitionRegistry::findCustomEvent(std::string_view identity) const {
  auto found = storage_->customEvents.find(std::string(identity));
  return found == storage_->customEvents.end() ? nullptr : found->second.get();
}
void DefinitionRegistry::addModule(std::string_view json) {
  for (const auto& value : parseJson(json).asArray()) define(Attribute::fromJson(value));
}
void DefinitionRegistry::define(const Attribute& attribute) {
  auto& storage = *storage_;
  if (attribute.name == "character") return defineCharacter(storage, attribute);
  if (attribute.name == "skill") return defineCharacterSkill(storage, attribute);
  if (attribute.name == "card") return defineCard(storage, attribute);
  if (attribute.name == "extension") return defineExtension(storage, attribute);
  OwnerType owner;
  if (attribute.name == "status") owner = OwnerType::Status;
  else if (attribute.name == "combatStatus") owner = OwnerType::CombatStatus;
  else if (attribute.name == "summon") owner = OwnerType::Summon;
  else if (attribute.name == "attachment") owner = OwnerType::Attachment;
  else throw std::runtime_error("Unknown root attribute " + attribute.name);
  EntityModel model(storage, owner);
  model.apply(attribute.attributes);
  if (model.reserved) return;
  model.finishSkills();
  auto& entries = owner == OwnerType::Attachment ? storage.attachments : storage.entities;
  entries.push_back(std::make_unique<EntityDefinition>(std::move(model.definition)));
}
DefinitionVersionResolver officialVersionResolver(std::string version) {
  if (versionIndex(version) < 0) throw std::runtime_error("Unknown game version " + version);
  return [version](int, const std::vector<VersionInfo>& versions) {
    int since = -1, until = -1;
    for (size_t index = 0; index < versions.size(); ++index) {
      const auto& candidate = versions[index];
      if (candidate.isSince && since == -1) since = static_cast<int>(index);
      if (!candidate.isSince && !versionLessThan(candidate.version, version) &&
          (until == -1 || versionLessThan(candidate.version, versions[until].version))) until = static_cast<int>(index);
    }
    if (since == -1 || versionLessThan(version, versions[since].version)) return -1;
    return until == -1 ? since : until;
  };
}
namespace {
template <typename T>
std::vector<const T*> resolveEntries(const std::vector<std::unique_ptr<T>>& entries, const std::vector<DefinitionVersionResolver>& resolvers) {
  std::vector<int> order;
  std::unordered_map<int, std::vector<const T*>> groups;
  for (const auto& entry : entries) {
    int id;
    if constexpr (std::is_same_v<T, CharacterEntry>) id = entry->definition.id;
    else id = entry->id;
    if (!groups.contains(id)) order.push_back(id);
    groups[id].push_back(entry.get());
  }
  std::vector<const T*> result;
  for (int id : order) {
    const auto& candidates = groups.at(id);
    std::vector<VersionInfo> versions;
    for (const T* entry : candidates) {
      if constexpr (std::is_same_v<T, CharacterEntry>) versions.push_back(entry->definition.version);
      else versions.push_back(entry->version);
    }
    for (const auto& resolver : resolvers) {
      int chosen = resolver(id, versions);
      if (chosen >= 0) {
        if (static_cast<size_t>(chosen) >= candidates.size()) throw std::runtime_error("Version resolver returned invalid index");
        result.push_back(candidates[chosen]);
        break;
      }
    }
  }
  return result;
}
}  // namespace
GameData DefinitionRegistry::resolve(const std::string& version) { return resolve(std::vector{officialVersionResolver(version)}); }
GameData DefinitionRegistry::resolve(const std::vector<DefinitionVersionResolver>& resolvers) {
  auto& storage = *storage_;
  GameData data;
  for (auto* entry : resolveEntries(storage.extensions, resolvers)) data.extensions.add(entry);
  for (auto* entry : resolveEntries(storage.entities, resolvers)) data.entities.add(entry);
  for (auto* entry : resolveEntries(storage.attachments, resolvers)) data.attachments.add(entry);
  std::unordered_map<int, const InitiativeEntry*> initiatives;
  std::unordered_map<int, const PassiveEntry*> passives;
  for (auto* entry : resolveEntries(storage.initiatives, resolvers)) initiatives[entry->id] = entry;
  for (auto* entry : resolveEntries(storage.passives, resolvers)) passives[entry->id] = entry;
  for (auto* entry : resolveEntries(storage.characters, resolvers)) {
    auto character = std::make_unique<CharacterDefinition>(entry->definition);
    for (int id : entry->skillIds) {
      if (initiatives.contains(id)) character->skills.push_back(initiatives.at(id)->skill);
      else if (passives.contains(id)) {
        const auto* passive = passives.at(id);
        for (const auto& [name, config] : passive->varConfigs) {
          if (character->findVarConfig(name)) throw std::runtime_error("Passive variables overlap: " + nameText(name));
          character->varConfigs.emplace_back(name, config);
        }
        character->skills.insert(character->skills.end(), passive->skills.begin(), passive->skills.end());
      }
    }
    character->associatedNightsoulsBlessing = data.entities.find(entry->nightsoulId);
    data.characters.add(character.get());
    storage.resolvedCharacters.push_back(std::move(character));
  }
  return data;
}
}  // namespace gitcg
