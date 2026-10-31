// Extension mutations run through SkillContext with the extension id installed in its options.
#include "definitions/skills/skill_model.h"
#include <stdexcept>

namespace gitcg {
void defineExtension(DefinitionStorage& storage, const Attribute& root) {
  auto extension = std::make_unique<ExtensionDefinition>();
  for (const auto& attribute : root.attributes) {
    const auto& name = attribute.name; const auto& value = attribute.argument(0);
    if (name == "idHint") extension->id = 50000000 + value.asInt();
    else if (name == "initialState") extension->initialState = value;
    else if (name == "schema" || name == "description") {}
    else if (name == "mutateWhen") {
      SkillModel model(storage, OwnerType::Extension);
      model.options->associatedExtensionId = extension->id;
      model.id = storage.nextSubId(extension->id);
      auto slots = storage.slots; int slot = lambdaSlot(attribute.argument(1));
      model.action = [slots, slot](SkillContext& c) {
        c.setExtensionState([&](JsonValue& state) { slots->call(slot, {&c, c.rawState(), c.self().id(), &c.eventArgument(), &state}); });
      };
      SkillDefinition skill;
      skill.id = model.id; skill.ownerType = OwnerType::Extension;
      skill.triggerOn = detailedEventName(value.asString());
      skill.action = model.buildAction(); skill.filter = model.buildFilter();
      extension->skills.push_back(storage.keepSkill(std::move(skill)));
    } else throw std::runtime_error("Unknown extension attribute " + name);
  }
  storage.extensions.push_back(std::move(extension));
}
}  // namespace gitcg
