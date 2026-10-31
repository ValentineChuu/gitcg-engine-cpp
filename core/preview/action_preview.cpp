#include "core/preview/action_preview.h"

#include <algorithm>
#include <array>
#include "core/mutations/mutation_observer.h"
#include "core/mutations/state_mutations.h"
#include "core/rules/entity_lookup.h"
#include "core/skills/skill_executor.h"
#include "core/values/engine_errors.h"

namespace gitcg {
namespace {

class PreviewCollector {
 public:
  explicit PreviewCollector(bool enabled) : previous_(mutationObserver), observer_([this](JsonValue mutation) { add(std::move(mutation)); }) {
    if (enabled) mutationObserver = &observer_;
  }
  ~PreviewCollector() { mutationObserver = previous_; }
  std::optional<int> mainDamageTarget;
  JsonValue result() const {
    JsonValue result = JsonValue::array();
    auto push = [&](const JsonValue& mutation) {
      JsonValue entry = JsonValue::object(); entry["mutation"] = mutation; result.push(std::move(entry));
    };
    for (const auto& m : damage_) push(m);
    for (const auto& m : entities_) push(m);
    for (const auto& [id, m] : actives_) push(m);
    for (const auto& group : variables_) for (const auto& [id, m] : group) push(m);
    return result;
  }
 private:
  using Entries = std::vector<std::pair<int, JsonValue>>;
  static void set(Entries& entries, int id, JsonValue mutation, bool keepDirection = false) {
    for (auto& [key, old] : entries) {
      if (key != id) continue;
      if (keepDirection && old.at("value").at("direction").asInt() != 0)
        mutation["value"]["direction"] = old.at("value").at("direction");
      old = std::move(mutation); return;
    }
    entries.emplace_back(id, std::move(mutation));
  }
  void add(JsonValue mutation) {
    const std::string& kind = mutation.at("$case").asString();
    const JsonValue& value = mutation.at("value");
    if (kind == "damage" || kind == "applyAura") {
      if (kind == "damage" && value.at("isSkillMainDamage").asBool() && !mainDamageTarget)
        mainDamageTarget = value.at("targetId").asInt();
      damage_.push_back(std::move(mutation));
    } else if (kind == "switchActive") {
      int who = value.at("who").asInt(); set(actives_, who, std::move(mutation));
    } else if (kind == "modifyEntityVar") {
      const std::string group = value.at("_group").asString();
      int index = group == "health" ? 0 : group == "energy" ? 1 : group == "aura" ? 2 : group == "alive" ? 3 : 4;
      int id = value.at("entityId").asInt();
      std::erase_if(mutation["value"].asObject(), [](const auto& field) { return field.first == "_group"; });
      set(variables_[index], id, std::move(mutation), true);
    } else if (kind == "createEntity" || kind == "moveEntity" || kind == "removeEntity") {
      entities_.push_back(std::move(mutation));
    }
  }
  MutationObserver* previous_;
  MutationObserver observer_;
  std::vector<JsonValue> damage_, entities_;
  Entries actives_;
  std::array<Entries, 5> variables_;
};
}  // namespace

ActionInfo modifyAndPreview(const GameStatePointer& state, const ActionInfo& action, bool enablePreview) {
  if (action.validity != action_validity::Valid) return action;
  auto shared = std::make_shared<ActionInfo>(action);
  auto precalculated = std::make_shared<ModifyActionEventArg>(state, shared);
  Mutator context(state);
  PreviewCollector collector(enablePreview);
  for (EventName stage : {EventName::ModifyAction0, EventName::ModifyAction1, EventName::ModifyAction2,
                          EventName::ModifyAction3, EventName::ModifyAction4}) {
    try { SkillExecutor(context, SkillEnvironment::Precalculate, nullptr).handleEvent(stage, precalculated); }
    catch (const IoNotProvidedError&) {}
  }
  ActionInfo result = precalculated->action();
  if (!enablePreview) return result;
  bool stopped = false;
  auto preview = [&](const std::function<void()>& body) {
    if (stopped) return;
    try { body(); } catch (const PreviewAbortedError&) { stopped = true; }
    catch (const IoNotProvidedError&) { stopped = true; }
  };
  auto event = [&](EventName name, std::shared_ptr<SkillArgument> argument) {
    preview([&] { SkillExecutor(context, SkillEnvironment::Preview, nullptr).handleEvent(name, argument); });
  };
  auto skill = [&] {
    InitiativeSkillArgument argument(result.targets);
    preview([&] { SkillExecutor(context, SkillEnvironment::Preview, nullptr).finalizeSkill(*result.skill, argument); });
  };
  auto details = std::make_shared<ActionInfo>(result);
  switch (result.type) {
    case ActionType::UseSkill: {
      EntityArea area = getEntityArea(*context.state(), activeCharacter(*context.state()->players[action.who])->id);
      event(EventName::OnBeforeUseSkill, std::make_shared<UseSkillEventArg>(context.state(), area, result.skill));
      skill();
      event(EventName::OnUseSkill, std::make_shared<UseSkillEventArg>(context.state(), area, result.skill));
      result.mainDamageTargetId = collector.mainDamageTarget;
      break;
    }
    case ActionType::PlayCard: {
      if (result.cost.countOf(dice::Legend) > 0) setPlayerFlag(context.mutableState(), action.who, PlayerFlag::LegendUsed, true);
      event(EventName::OnBeforePlayCard, std::make_shared<PlayCardEventArg>(context.state(), details));
      if (result.willBeEffectless) removeEntity(context.mutableState(), result.skill->caller.id(), action.who, RemoveReason::EventCardPlayNoEffect);
      else skill();
      event(EventName::OnPlayCard, std::make_shared<PlayCardEventArg>(context.state(), details));
      break;
    }
    case ActionType::SwitchActive: {
      EventList events = context.switchActive(action.who, result.to);
      for (const auto& entry : events.items()) event(entry.name, entry.argument);
      break;
    }
    case ActionType::ElementalTuning: {
      EntityArea from{AreaType::Hands, action.who, 0, result.card->id};
      auto disposed = std::make_shared<DisposeEventArg>(context.state(), result.card, RemoveReason::ElementalTuning, from, nullptr);
      removeEntity(context.mutableState(), result.card->id, action.who, RemoveReason::ElementalTuning);
      event(EventName::OnDispose, disposed);
      break;
    }
    case ActionType::DeclareEnd:
      setPlayerFlag(context.mutableState(), action.who, PlayerFlag::DeclaredEnd, true);
      event(EventName::OnDeclareEnd, std::make_shared<PlayerEventArg>(context.state(), action.who));
      break;
  }
  event(EventName::OnAction, std::make_shared<ActionEventArg>(context.state(), details));
  result.preview = collector.result();
  return result;
}
}  // namespace gitcg
