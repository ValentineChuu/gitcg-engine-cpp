// Mutable GTS entity model shared by statuses, equipment, passive skills and attachments.
#pragma once
#include "definitions/registry/storage.h"

namespace gitcg {
struct EntityModel {
  DefinitionStorage& storage;
  double id = 0;
  OwnerType owner = OwnerType::Status;
  EntityDefinition definition;
  std::shared_ptr<ContextOptions> options = std::make_shared<ContextOptions>();
  std::vector<SkillOperation> stagedOperations;
  std::vector<std::pair<Name, JsonValue>> declaredUsages;
  int usagePerRoundIndex = 0;
  bool reserved = false, hasVersion = false;
  std::optional<JsonValue> target;
  EntityModel(DefinitionStorage& storage, OwnerType owner, double id = 0, std::shared_ptr<ContextOptions> options = nullptr);
  void apply(const std::vector<Attribute>& attributes);
  void apply(const Attribute& attribute);
  void setVariable(std::string name, double value, const JsonValue& options);
  Name setUsage(double count, JsonValue options, bool perRound);
  void finishSkills();
  void addBuiltin(std::string event, SkillOperation action, std::function<bool(SkillContext&)> filter = {}, std::string listenTo = "sameArea", bool bypass = false, bool hand = false, bool pile = false);
};
VariableConfig createVariableConfig(double initial, const JsonValue& options);
EntityType entityType(OwnerType owner);
SkillType skillType(std::string_view text);
void addDescription(EntityModel& model, std::string key, const JsonValue& value);
}  // namespace gitcg
