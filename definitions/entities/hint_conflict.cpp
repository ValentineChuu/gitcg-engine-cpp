// Hint conversion presets and same-area/cross-character conflict disposal.
#include "definitions/entities/builtins.h"
#include "definitions/skills/skill_model.h"
#include "core/rules/entity_lookup.h"
#include <algorithm>

namespace gitcg {
void addHint(EntityModel& model, const Attribute& attribute) {
  auto options = attributeOptions(attribute.attributes);
  if (const auto* preset = options.find("dynamicPreset")) {
    bool swirl = preset->asString() == "swirled";
    SkillModel skill(model.storage, model.owner, model.options);
    skill.id = model.storage.nextSubId(model.id);
    skill.userFilters.push_back([swirl](SkillContext& c) {
      auto& e = c.e<DamageOrHealEventArg>();
      if (swirl) return (e.source().isCharacter() || e.source().entity()->definition->type == EntityType::Summon) && e.isSwirl() != -1;
      return e.type() >= damage::Cryo && e.type() <= damage::Electro;
    });
    Attribute usage{"usage", {1}, {{"name", {swirl ? "swirledUsage" : "chpeDamagedUsage"}, {}}, {"visible", {false}, {}}}};
    skill.setUsage(model, usage);
    skill.action = [swirl](SkillContext& c) { c.setVariable(intern("hintIcon"), swirl ? c.e<DamageOrHealEventArg>().isSwirl() : c.e<DamageOrHealEventArg>().type()); };
    model.definition.skills.push_back(model.storage.keepSkill(skill.buildTriggered(model, swirl ? "dealDamage" : "damaged")));
  }
  auto hidden = JsonValue::object(); hidden["visible"] = false;
  model.setVariable("hintIcon", attribute.argument(0).asNumber(), hidden);
  const auto& text = attribute.argument(1);
  if (lambdaSlot(text)) {
    model.definition.hintText = "${[GCG_TOKEN_HINT_TEXT]}";
    addDescription(model, "[GCG_TOKEN_HINT_TEXT]", text);
  } else if (text.isNumber()) {
    model.definition.hintText = jsString(text);
  } else if (text.isString()) model.definition.hintText = text.asString();
}
void addConflict(EntityModel& model, const Attribute& attribute) {
  std::vector<int> ids{static_cast<int>(model.id)};
  bool cross = attribute.argument(0).isString();
  for (size_t index = cross ? 1 : 0; index < attribute.positionals.size(); ++index) ids.push_back(attribute.positionals[index].asInt());
  model.addBuiltin("selfEnter", [ids, cross](SkillContext& c) {
    auto selfArea = c.entityArea(c.self().id());
    std::vector<int> disposing;
    for (const auto& entry : getAllEntities(c.state())) {
      if (!entry.state.isEntity() || entry.state.id() == c.self().id() || selfArea.who != entry.area.who) continue;
      if (std::find(ids.begin(), ids.end(), entry.state.entity()->definition->id) == ids.end()) continue;
      if (entry.area.type == AreaType::Characters && selfArea.type == AreaType::Characters) {
        if (cross || entry.area.characterId == selfArea.characterId) disposing.push_back(entry.state.id());
      } else if (entry.area.type == selfArea.type) disposing.push_back(entry.state.id());
    }
    for (int id : disposing) c.dispose(std::vector<AnyHandle>{c.get(id)});
  });
}
}  // namespace gitcg
