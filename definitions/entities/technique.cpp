// Technique skills have their own context options; their usage variables belong to the equipment model.
#include "definitions/entities/builtins.h"
#include "definitions/skills/skill_model.h"

namespace gitcg {
void addTechniqueSkill(EntityModel& model, const Attribute& attribute) {
  SkillModel skill(model.storage, OwnerType::Equipment);
  skill.type = SkillType::Technique;
  skill.apply(attribute.attributes, &model);
  if (skill.id == 0) skill.id = model.storage.nextSubId(model.id);
  model.definition.skills.push_back(model.storage.keepSkill(skill.buildInitiative()));
}
void addTechniqueNightsoul(EntityModel& model, const JsonValue& options) {
  model.definition.obtainable = false;
  model.addBuiltin("beforeAction", [](SkillContext& c) {
    auto blessing = c.selfEntity().master().hasNightsoulsBlessing();
    if (blessing && blessing->getVariable(intern("nightsoul")) <= 0) c.dispose();
  }, {}, "all");
  if (optionBool(options, "alsoDisposeNightsoulsBlessing", true)) model.addBuiltin("selfDispose", [](SkillContext& c) {
    auto area = c.e<DisposeEventArg>().area;
    if (area.type != AreaType::Characters) return;
    auto blessing = CharacterHandle(&c, area.characterId).hasNightsoulsBlessing();
    if (blessing) blessing->dispose();
  });
}
}  // namespace gitcg
