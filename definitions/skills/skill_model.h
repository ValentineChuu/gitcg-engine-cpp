// Common action/filter composition for initiative, triggered and technique skills.
#pragma once
#include "definitions/entities/entity_model.h"

namespace gitcg {
using ContextFilter = std::function<bool(SkillContext&)>;
using ContextTargets = std::function<std::vector<AnyState>(SkillContext&)>;
struct SkillModel {
  DefinitionStorage& storage;
  OwnerType owner;
  double id = 0;
  SkillType type = SkillType::None;
  std::shared_ptr<ContextOptions> options;
  std::vector<SkillOperation> pre, post;
  SkillOperation action = [](SkillContext&) {};
  std::vector<ContextFilter> filters, userFilters;
  std::vector<ContextTargets> targets;
  InitiativeSkillConfig config;
  std::optional<Name> usage, perRound;
  bool decreaseUsage = false, decreasePerRound = false;
  std::optional<std::string> listenOverride;
  bool bypassOverride = false, pileOverride = false;
  SkillModel(DefinitionStorage& storage, OwnerType owner, std::shared_ptr<ContextOptions> options = nullptr);
  void apply(const std::vector<Attribute>& attributes, EntityModel* caller = nullptr);
  void setUsage(EntityModel& caller, const Attribute& attribute);
  SkillDefinition buildInitiative();
  SkillDefinition buildTriggered(EntityModel& caller, const JsonValue& event, std::string listenTo = "sameArea", bool bypass = false, bool hand = false, bool pile = false, bool stage = true);
  SkillAction buildAction();
  SkillFilter buildFilter();
  void usageOperations();
};
ContextTargets slotTargets(std::shared_ptr<LambdaSlots> slots, const JsonValue& value);
EventName detailedEventName(std::string_view detailed);
bool detailedEventFilter(std::string_view detailed, std::string_view listenTo, SkillContext& context);
void defineCharacter(DefinitionStorage& storage, const Attribute& root);
void defineCharacterSkill(DefinitionStorage& storage, const Attribute& root);
void defineCard(DefinitionStorage& storage, const Attribute& root);
void defineExtension(DefinitionStorage& storage, const Attribute& root);
}  // namespace gitcg
