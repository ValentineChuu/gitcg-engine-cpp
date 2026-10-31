// Source-ordered skill attributes and SkillContext wrappers, including usage filters and post-actions.
#include "definitions/skills/skill_model.h"
#include "core/rules/costs.h"
#include "core/rules/entity_lookup.h"
#include <stdexcept>

namespace gitcg {
SkillType skillType(std::string_view text) {
  if (text == "normal") return SkillType::Normal;
  if (text == "elemental") return SkillType::Elemental;
  if (text == "burst") return SkillType::Burst;
  if (text == "technique") return SkillType::Technique;
  if (text == "playCard") return SkillType::PlayCard;
  throw std::runtime_error("Unknown skill type " + std::string(text));
}
SkillModel::SkillModel(DefinitionStorage& storage, OwnerType owner, std::shared_ptr<ContextOptions> options)
  : storage(storage), owner(owner), options(options ? options : std::make_shared<ContextOptions>()) {}
ContextTargets slotTargets(std::shared_ptr<LambdaSlots> slots, const JsonValue& value) {
  int slot = lambdaSlot(value);
  if (!slot) throw std::runtime_error("Expected target lambda slot");
  return [slots, slot](SkillContext& context) {
    auto result = slots->call(slot, context);
    std::vector<AnyState> targets;
    for (const auto& id : result.asArray()) targets.push_back(getEntityById(context.state(), id.asInt()));
    return targets;
  };
}
void SkillModel::setUsage(EntityModel& caller, const Attribute& attribute) {
  auto usageOptions = attributeOptions(attribute.attributes);
  bool isPerRound = attribute.argument(0).isString() && attribute.argument(0).asString() == "perRound";
  if (isPerRound && !usageOptions.find("visible")) usageOptions["visible"] = false;
  Name name = caller.setUsage(attribute.argument(isPerRound ? 1 : 0).asNumber(), usageOptions, isPerRound);
  auto& variable = isPerRound ? perRound : usage;
  if (variable) throw std::runtime_error("Cannot specify usage multiple times for the same skill");
  variable = name;
  (isPerRound ? decreasePerRound : decreaseUsage) = optionBool(usageOptions, "autoDecrease", true);
  userFilters.insert(userFilters.begin(), [name](SkillContext& context) { return context.getVariable(name) > 0; });
}
void SkillModel::apply(const std::vector<Attribute>& attributes, EntityModel* caller) {
  for (const auto& attribute : attributes) {
    const auto& name = attribute.name;
    const auto& value = attribute.argument(0);
    if (name == "id") id = value.asNumber();
    else if (name == "cost") config.requiredCost.set(value.asInt(), attribute.argument(1).asInt());
    else if (name == "associateExtension") options->associatedExtensionId = value.asInt();
    else if (name == "prepared") { config.omitEvents = true; config.gainEnergy = false; config.hidden = true; }
    else if (name == "abortPreview") pre.push_back([](SkillContext& c) { c.abortPreview(); });
    else if (name == "hidden") config.hidden = true;
    else if (name == "noEnergy") config.gainEnergy = false;
    else if (name == "forcePlunging") config.alwaysPlunging = true;
    else if (name == "forceCharged") config.alwaysCharged = true;
    else if (name == "asSkillType") type = skillType(value.asString());
    else if (name == "~action") {
      int slot = lambdaSlot(value); auto slots = storage.slots;
      action = [slots, slot](SkillContext& context) { slots->call(slot, context); };
    } else if (name == "when" || name == "filter") {
      int slot = lambdaSlot(value); auto slots = storage.slots;
      userFilters.push_back([slots, slot](SkillContext& context) { return jsTruthy(slots->call(slot, context)); });
    } else if (name == "addTarget") targets.push_back(slotTargets(storage.slots, value));
    else if (name == "usage" && caller) setUsage(*caller, attribute);
    else if (name == "listenTo") listenOverride = value.asString();
    else if (name == "bypassDefeatedFilter") bypassOverride = true;
    else if (name == "enablePileTriggering") pileOverride = true;
    else throw std::runtime_error("Unknown skill attribute " + name);
  }
}
void SkillModel::usageOperations() {
  if (decreasePerRound) post.push_back([](SkillContext& context) { context.consumeUsagePerRound(); });
  if (decreaseUsage) {
    Name name = *usage;
    if (name == intern("usage")) post.push_back([](SkillContext& context) { context.consumeUsage(); });
    else post.push_back([name](SkillContext& context) { context.addVariable(name, -1); });
  }
}
SkillAction SkillModel::buildAction() {
  auto operations = pre;
  operations.push_back(action);
  operations.insert(operations.end(), post.begin(), post.end());
  return SkillContext::encapsulate(options, [operations](SkillContext& context) { for (const auto& operation : operations) operation(context); });
}
SkillFilter SkillModel::buildFilter() {
  auto all = filters;
  all.insert(all.end(), userFilters.begin(), userFilters.end());
  return SkillContext::encapsulateForResult<bool>(options, [all](SkillContext& context) {
    for (const auto& filter : all) if (!filter(context)) return false;
    return true;
  });
}
SkillDefinition SkillModel::buildInitiative() {
  usageOperations();
  SkillDefinition result;
  result.id = id; result.ownerType = owner; result.skillType = type;
  result.usagePerRoundVariableName = perRound;
  result.action = buildAction(); result.filter = buildFilter();
  config.requiredCost = normalizeCost(config.requiredCost);
  config.costSize = costSize(config.requiredCost);
  config.diceCostSize = diceCostSize(config.requiredCost);
  auto getters = targets; auto contextOptions = options;
  config.getTarget = [getters, contextOptions](const GameStatePointer& state, const SkillInfo& info) {
    std::vector<InitiativeSkillArgument> result;
    std::function<void(size_t, InitiativeSkillArgument)> generate = [&](size_t index, InitiativeSkillArgument known) {
      if (index == getters.size()) { result.push_back(std::move(known)); return; }
      SkillContext context(state, info, known, *contextOptions);
      for (const auto& target : getters[index](context)) {
        InitiativeSkillArgument next = known;
        next.targets.push_back(target);
        generate(index + 1, std::move(next));
      }
    };
    generate(0, {});
    return result;
  };
  result.initiativeSkillConfig = config;
  return result;
}
SkillDefinition SkillModel::buildTriggered(EntityModel& caller, const JsonValue& event, std::string listenTo, bool bypass, bool hand, bool pile, bool stage) {
  usageOperations();
  if (listenOverride) listenTo = *listenOverride;
  bypass = bypass || bypassOverride;
  pile = pile || pileOverride;
  const CustomEvent* custom = event.isObject() ? storage.customEvent(event) : nullptr;
  std::string detailed = custom ? "customEvent" : event.asString();
  if (detailed != "selfDiscard" && detailed != "selfDispose" && detailed != "selfHandCardInserted")
    filters.push_back([](SkillContext& c) { return c.entityArea(c.self().id()).type != AreaType::RemovedEntities; });
  if (!hand && owner != OwnerType::Attachment) filters.push_back([](SkillContext& c) { return c.entityArea(c.self().id()).type != AreaType::Hands; });
  if (!pile && owner != OwnerType::Attachment) filters.push_back([](SkillContext& c) { return c.entityArea(c.self().id()).type != AreaType::Pile; });
  if (!stage) filters.push_back([](SkillContext& c) { auto area = c.entityArea(c.self().id()).type; return area == AreaType::Hands || area == AreaType::Pile || area == AreaType::RemovedEntities; });
  if (owner == OwnerType::Character && detailed != "defeated" && !bypass)
    filters.push_back([](SkillContext& c) { return c.getVariable(intern("alive")) != 0; });
  if ((owner == OwnerType::Status || owner == OwnerType::Equipment) && !bypass)
    filters.push_back([](SkillContext& c) { return c.entityArea(c.self().id()).type != AreaType::Characters || c.selfEntity().master().alive(); });
  filters.push_back([detailed, listenTo](SkillContext& c) { return detailedEventFilter(detailed, listenTo, c); });
  if (custom) filters.push_back([custom](SkillContext& c) { return c.e<CustomEventEventArg>().customEvent == custom; });
  SkillDefinition result;
  result.id = id; result.ownerType = owner; result.skillType = type;
  result.triggerOn = detailedEventName(detailed);
  result.usagePerRoundVariableName = perRound;
  result.action = buildAction(); result.filter = buildFilter();
  return result;
}
}  // namespace gitcg
