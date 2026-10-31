// Prepare and shield attributes create executable skills rather than card-code slots.
#include "definitions/entities/builtins.h"
#include "core/events/action_event_args.h"
#include "core/rules/entity_lookup.h"
#include <algorithm>

namespace gitcg {
void addPrepare(EntityModel& model, const Attribute& attribute) {
  auto options = attributeOptions(attribute.attributes);
  if (const auto* count = options.find("hintCount")) {
    auto hidden = JsonValue::object(); hidden["visible"] = false;
    model.setVariable("hintCount", count->asNumber(), hidden);
  }
  model.definition.tags.push_back(intern("preparingSkill"));
  const auto skill = attribute.argument(0);
  model.addBuiltin("replaceActionBySkill", [options, skill](SkillContext& c) {
    if (const auto* next = options.find("nextStatus")) {
      CreateEntityOptions createOptions;
      if (const auto* value = options.find("nextStatusCreateOpt")) {
        if (auto* variables = value->find("overrideVariables")) for (const auto& [name, value] : variables->asObject()) createOptions.overrideVariables.emplace_back(intern(name), value.asNumber());
      }
      c.characterStatus(next->asInt(), {c.get(c.selfEntity().master().id())}, createOptions);
    }
    if (skill.isString()) c.useNormalSkill(true); else c.useSkill(skill.asNumber(), true);
    c.dispose();
  });
  model.addBuiltin("switchActive", [](SkillContext& c) { c.dispose(); }, [](SkillContext& c) {
    auto from = c.e<SwitchActiveEventArg>().switchInfo.from;
    return from && from->id == c.selfEntity().master().id();
  });
}
void addShield(EntityModel& model, const Attribute& attribute) {
  const double count = attribute.argument(0).asNumber();
  const auto& maximum = attribute.argument(1);
  auto options = JsonValue::object();
  options["append"] = maximum.isNull() ? count : maximum.isString() ? DEFAULT_VARIABLE_UPPER_BOUND : maximum.asNumber();
  model.definition.tags.push_back(intern("shield"));
  model.setVariable("shield", count, options);
  model.addBuiltin("decreaseDamaged", [](SkillContext& c) {
    auto& event = c.e<ModifyDamageEventArg>();
    double shield = c.getVariable(intern("shield")), current = event.value(), decreased = std::min(shield, current);
    event.decreaseDamage(decreased);
    c.addVariable(intern("shield"), -decreased);
    if (shield <= current) c.dispose();
  }, [](SkillContext& c) {
    return c.selfEntity().definition().type != EntityType::CombatStatus ||
      c.e<ModifyDamageEventArg>().target()->id == c.state().players[c.e<ModifyDamageEventArg>().targetWho]->activeCharacterId;
  });
}
void addNightsoul(EntityModel& model, const Attribute& attribute) {
  auto options = JsonValue::object(); options["append"] = true; options["range"] = attribute.argument(0);
  auto incoming = attributeOptions(attribute.attributes);
  for (const auto& [name, value] : incoming.asObject()) options[name] = value;
  model.definition.tags.push_back(intern("nightsoulsBlessing"));
  model.setVariable("nightsoul", 0, options);
  if (optionBool(options, "autoDispose", false)) model.addBuiltin("beforeAction", [](SkillContext& c) { c.dispose(); }, [](SkillContext& c) {
    return c.getVariable(intern("nightsoul")) <= 0;
  }, "all");
}
}  // namespace gitcg
