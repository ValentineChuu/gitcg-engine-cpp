#include "cards/translated/translated_card_support.h"

#include "core/values/engine_errors.h"

namespace gitcg::js {

std::string typeNameOf(const EntityDefinition& definition) {
  switch (definition.type) {
    case EntityType::EventCard: return "eventCard";
    case EntityType::Status: return "status";
    case EntityType::CombatStatus: return "combatStatus";
    case EntityType::Equipment: return "equipment";
    case EntityType::Support: return "support";
    case EntityType::Summon: return "summon";
    case EntityType::Attachment: return "attachment";
  }
  return "";
}

namespace {
const DefinitionRegistry* customEventRegistry = nullptr;
}  // namespace

void useCustomEventRegistry(const DefinitionRegistry* registry) {
  customEventRegistry = registry;
}

const CustomEvent* customEvent(const char* identity) {
  const CustomEvent* event = customEventRegistry ? customEventRegistry->findCustomEvent(identity) : nullptr;
  if (!event) throw GameDataError(std::string("unknown custom event ") + identity);
  return event;
}

PileInsertion pileInsertion(const std::string& strategy) {
  if (strategy == "top") return {PileStrategy::Top, 0};
  if (strategy == "bottom") return {PileStrategy::Bottom, 0};
  if (strategy == "random") return {PileStrategy::Random, 0};
  if (strategy == "spaceAround") return {PileStrategy::SpaceAround, 0};
  if (strategy.starts_with("topRange")) return {PileStrategy::TopRange, std::stoi(strategy.substr(8))};
  if (strategy.starts_with("topIndex")) return {PileStrategy::TopIndex, std::stoi(strategy.substr(8))};
  throw GameDataError("Invalid strategy " + strategy);
}

std::vector<std::string> nameTexts(const std::vector<Name>& names) {
  std::vector<std::string> texts;
  for (Name name : names) texts.push_back(std::string(nameText(name)));
  return texts;
}

double variableOf(const AnyHandle& state, Name name) {
  return state.isCharacter() ? state.asCharacter().getVariable(name) : state.asEntity().getVariable(name);
}

}  // namespace gitcg::js
