// Sub-ids count globally per main id, including old definitions and reserved card play skills.
#include "definitions/registry/storage.h"
#include <stdexcept>
namespace gitcg {
double DefinitionStorage::nextSubId(double mainId) {
  static std::unordered_map<double, int> subIds;
  int counter = ++subIds[mainId];
  if (counter >= 100) throw std::runtime_error("Sub ID counter for " + std::to_string(mainId) + " exceeded 100");
  return mainId + 0.01 * counter;
}
const SkillDefinition* DefinitionStorage::keepSkill(SkillDefinition skill) {
  skills.push_back(std::make_unique<SkillDefinition>(std::move(skill)));
  return skills.back().get();
}
const CustomEvent* DefinitionStorage::customEvent(const JsonValue& value) {
  std::string name = value.at("$event").asString();
  auto* identity = value.find("identity");
  auto& event = customEvents[identity ? identity->asString() : name];
  if (!event) event = std::make_unique<CustomEvent>(CustomEvent{name});
  return event.get();
}
}  // namespace gitcg
