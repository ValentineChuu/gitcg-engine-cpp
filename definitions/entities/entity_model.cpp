// Entity model defaults, explicit attributes and implicit defeated/roundEnd skills.
#include "definitions/entities/entity_model.h"
#include "definitions/skills/skill_model.h"
#include "core/values/usage_per_round_names.h"
#include <algorithm>
#include <stdexcept>

namespace gitcg {
EntityType entityType(OwnerType owner) {
  switch (owner) {
    case OwnerType::EventCard: return EntityType::EventCard;
    case OwnerType::Status: return EntityType::Status;
    case OwnerType::CombatStatus: return EntityType::CombatStatus;
    case OwnerType::Equipment: return EntityType::Equipment;
    case OwnerType::Support: return EntityType::Support;
    case OwnerType::Summon: return EntityType::Summon;
    case OwnerType::Attachment: return EntityType::Attachment;
    default: return EntityType::Status;
  }
}
EntityModel::EntityModel(DefinitionStorage& storage, OwnerType owner, double id, std::shared_ptr<ContextOptions> contextOptions)
  : storage(storage), id(id), owner(owner) {
  if (contextOptions) options = std::move(contextOptions);
  definition.id = static_cast<int>(id);
  definition.type = entityType(owner);
  // JS AttachmentModel always supplies a modifier function, including when its list is empty.
  if (owner == OwnerType::Attachment) {
    definition.modifications = [](const GameStatePointer&, int) { return std::vector<AttachmentModification>{}; };
  }
  definition.disposeOnMasterDefeated = owner == OwnerType::Status || owner == OwnerType::Equipment;
}
void EntityModel::apply(const std::vector<Attribute>& attributes) { for (const auto& attribute : attributes) apply(attribute); }
void addDescription(EntityModel& model, std::string key, const JsonValue& value) {
  for (const auto& [existing, getter] : model.definition.descriptionDictionary) if (existing == key) throw std::runtime_error("Description key already exists: " + key);
  auto slots = model.storage.slots;
  int slot = lambdaSlot(value);
  auto extension = model.options->associatedExtensionId;
  model.definition.descriptionDictionary.emplace_back(key, [slots, slot, extension](const GameStatePointer& state, int id) {
    JsonValue* extensionState = nullptr;
    JsonValue snapshot;
    if (extension) for (const auto& ext : state->extensions) if (ext.definition->id == *extension) { snapshot = ext.state; extensionState = &snapshot; break; }
    JsonValue result = slots->call(slot, {nullptr, state, id, nullptr, extensionState});
    return jsString(result);
  });
}
void EntityModel::finishSkills() {
  if (definition.disposeOnMasterDefeated) {
    SkillModel skill(storage, owner, options);
    skill.id = storage.nextSubId(id);
    skill.action = [](SkillContext& c) { c.dispose(); };
    auto* built = storage.keepSkill(skill.buildTriggered(*this, "defeated", "sameArea", true));
    definition.skills.insert(definition.skills.begin(), built);
  }
  std::vector<Name> names;
  for (Name name : usagePerRoundNames()) if (definition.findVarConfig(name)) names.push_back(name);
  bool duration = definition.findVarConfig(intern("duration"));
  if (!names.empty() || duration) {
    addBuiltin("roundEnd", [names, duration](SkillContext& c) {
      const auto self = c.self().latest();
      for (Name name : names) {
        const VariableConfig* config = self.isCharacter() ? self.character()->definition->findVarConfig(name) : self.entity()->definition->findVarConfig(name);
        if (config) c.setVariable(name, config->initialValue);
      }
      if (duration) { c.addVariable(intern("duration"), -1); if (c.getVariable(intern("duration")) <= 0) c.dispose(); }
    });
  }
}
void EntityModel::addBuiltin(std::string event, SkillOperation action, ContextFilter filter, std::string listenTo, bool bypass, bool hand, bool pile) {
  SkillModel skill(storage, owner, options);
  skill.id = storage.nextSubId(id);
  skill.action = std::move(action);
  if (filter) skill.userFilters.push_back(std::move(filter));
  definition.skills.push_back(storage.keepSkill(skill.buildTriggered(*this, event, listenTo, bypass, hand, pile)));
}
}  // namespace gitcg
