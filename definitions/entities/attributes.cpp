// Entity attribute dispatch; feature-specific built-ins are implemented next to their own state and actions.
#include "definitions/entities/entity_model.h"
#include "definitions/entities/builtins.h"
#include "definitions/skills/skill_model.h"
#include <stdexcept>

namespace gitcg {
void EntityModel::apply(const Attribute& attribute) {
  const auto& name = attribute.name; const auto& value = attribute.argument(0);
  auto childOptions = attributeOptions(attribute.attributes);
  if (name == "id") { id = value.asNumber(); definition.id = value.asInt(); }
  else if (name == "reserved") reserved = true;
  else if (name == "since" || name == "until") { definition.version = {name == "since", value.asString()}; hasVersion = true; }
  else if (name == "associateExtension") options->associatedExtensionId = value.asInt();
  else if (name == "tags") for (const auto& tag : attribute.positionals) definition.tags.push_back(intern(tag.asString()));
  else if (name == "variable") setVariable(value.asString(), attribute.argument(1).asNumber(), childOptions);
  else if (name == "duration") setVariable("duration", value.asNumber(), childOptions);
  else if (name == "oneDuration") { childOptions["visible"] = false; setVariable("duration", 1, childOptions); }
  else if (name == "usage") setUsage(value.asNumber(), childOptions, false);
  else if (name == "noDefaultDispose") definition.disposeOnMasterDefeated = false;
  else if (name == "replaceDescription") addDescription(*this, value.asString(), attribute.argument(1));
  else if (name == "defineSnippet") {
    std::string snippetName = value.isString() ? value.asString() : "default";
    for (const auto& child : attribute.attributes) {
      if (child.name != "~action") throw std::runtime_error("Unknown snippet attribute " + child.name);
      int slot = lambdaSlot(child.argument(0)); auto slots = storage.slots;
      options->snippets[snippetName] = [slots, slot](SkillContext& c) { slots->call(slot, c); };
    }
  } else if (name == "on" || name == "once") {
    if (value.isString() && value.asString() == "staged") {
      for (const auto& child : attribute.attributes) {
        if (child.name != "~action") throw std::runtime_error("Unknown staged attribute " + child.name);
        int slot = lambdaSlot(child.argument(0)); auto slots = storage.slots;
        stagedOperations.push_back([slots, slot](SkillContext& c) { slots->call(slot, c); });
      }
    } else {
      SkillModel skill(storage, owner, options);
      skill.apply(attribute.attributes, this);
      skill.id = storage.nextSubId(id);
      if (name == "once") {
        Attribute usage{"usage", {1}, {{"visible", {false}, {}}}};
        skill.setUsage(*this, usage);
      }
      definition.skills.push_back(storage.keepSkill(skill.buildTriggered(*this, value)));
    }
  } else if (name == "adventureSpot") {
    definition.obtainable = false; definition.tags.push_back(intern("adventureSpot"));
    JsonValue options = JsonValue::object(); options["append"] = true; setVariable("exp", 1, options);
  } else if (name == "target") target = value;
  else if (name == "skill") addTechniqueSkill(*this, attribute);
  else if (name == "nightsoul") addTechniqueNightsoul(*this, childOptions);
  else if (name == "prepare") addPrepare(*this, attribute);
  else if (name == "shield") addShield(*this, attribute);
  else if (name == "nightsoulsBlessing") addNightsoul(*this, attribute);
  else if (name == "elementalBlessing") addElementalBlessing(*this, attribute);
  else if (name == "hint") addHint(*this, attribute);
  else if (name == "conflictWith") addConflict(*this, attribute);
  else if (owner == OwnerType::Attachment) addAttachmentModification(*this, attribute);
  else throw std::runtime_error("Unknown entity attribute " + name);
}
}  // namespace gitcg
