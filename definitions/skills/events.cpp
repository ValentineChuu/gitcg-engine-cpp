// Detailed event descriptors from runtime/skill.ts. Relations use the event's snapshot, caller area stays live.
#include "definitions/skills/skill_model.h"
#include "core/events/action_event_args.h"
#include "core/rules/entity_lookup.h"
#include "core/rules/initiative_skills.h"
#include <stdexcept>
#include <unordered_map>

namespace gitcg {
EventName detailedEventName(std::string_view detailed) {
  static const std::unordered_map<std::string_view, EventName> names = {
    {"roll", EventName::ModifyRoll},
    {"addDice", EventName::ModifyAction0},
    {"deductElementDice", EventName::ModifyAction1},
    {"deductOmniDice", EventName::ModifyAction2},
    {"deductOmniDiceSwitch", EventName::ModifyAction2},
    {"deductOmniDiceCard", EventName::ModifyAction2},
    {"deductAllDiceCard", EventName::ModifyAction3},
    {"deductVoidDiceSkill", EventName::ModifyAction0},
    {"deductElementDiceSkill", EventName::ModifyAction1},
    {"deductOmniDiceSkill", EventName::ModifyAction2},
    {"deductOmniDiceTechnique", EventName::ModifyAction2},
    {"beforeFastSwitch", EventName::ModifyAction4},
    {"modifySkillDamageType", EventName::ModifyDamage0},
    {"increaseDamage", EventName::ModifyDamage1},
    {"increaseSkillDamage", EventName::ModifyDamage1},
    {"increaseTechniqueDamage", EventName::ModifyDamage1},
    {"multiplySkillDamage", EventName::ModifyDamage2},
    {"increaseDamaged", EventName::ModifyDamage1},
    {"multiplyDamaged", EventName::ModifyDamage2},
    {"decreaseDamaged", EventName::ModifyDamage3},
    {"cancelHealed", EventName::ModifyHeal0},
    {"decreaseHealed", EventName::ModifyHeal1},
    {"beforeDefeated", EventName::ModifyZeroHealth},
    {"battleBegin", EventName::OnBattleBegin},
    {"roundBegin", EventName::OnRoundBegin},
    {"roundEnd", EventName::OnRoundEnd},
    {"actionPhase", EventName::OnActionPhase},
    {"endPhase", EventName::OnEndPhase},
    {"beforeAction", EventName::OnBeforeAction},
    {"replaceActionBySkill", EventName::ReplaceAction},
    {"action", EventName::OnAction},
    {"playCard", EventName::OnPlayCard},
    {"beforeSkill", EventName::OnBeforeUseSkill},
    {"beforeTechnique", EventName::OnBeforeUseSkill},
    {"useSkill", EventName::OnUseSkill},
    {"useTechnique", EventName::OnUseSkill},
    {"useSkillOrTechnique", EventName::OnUseSkill},
    {"declareEnd", EventName::OnDeclareEnd},
    {"switchActive", EventName::OnSwitchActive},
    {"drawCard", EventName::OnHandCardInserted},
    {"handCardInserted", EventName::OnHandCardInserted},
    {"selfHandCardInserted", EventName::OnHandCardInserted},
    {"discard", EventName::OnDispose},
    {"discardOrTuneCard", EventName::OnDispose},
    {"selfDiscard", EventName::OnDispose},
    {"dealDamage", EventName::OnDamageOrHeal},
    {"skillDamage", EventName::OnDamageOrHeal},
    {"damaged", EventName::OnDamageOrHeal},
    {"healed", EventName::OnDamageOrHeal},
    {"damagedOrHealed", EventName::OnDamageOrHeal},
    {"modifyReaction", EventName::ModifyReaction},
    {"reaction", EventName::OnReaction},
    {"dealReaction", EventName::OnReaction},
    {"selfEnter", EventName::OnEnter},
    {"entityEnter", EventName::OnEnter},
    {"selfDispose", EventName::OnDispose},
    {"entityDispose", EventName::OnDispose},
    {"defeated", EventName::OnDamageOrHeal},
    {"revive", EventName::OnRevive},
    {"transformDefinition", EventName::OnTransformDefinition},
    {"generateDice", EventName::OnGenerateDice},
    {"cancelConsumeNightsoul", EventName::ModifyChangeVariable},
    {"consumeNightsoul", EventName::OnChangeVariable},
    {"gainNightsoul", EventName::OnChangeVariable},
    {"consumeUsage", EventName::OnChangeVariable},
    {"gainUsage", EventName::OnChangeVariable},
    {"consumeEffect", EventName::OnChangeVariable},
    {"gainEffect", EventName::OnChangeVariable},
    {"selectCard", EventName::OnSelectCard},
    {"adventure", EventName::OnAdventure},
    {"customEvent", EventName::OnCustomEvent},
  };
  if (auto found = names.find(detailed); found != names.end()) return found->second;
  for (int index = static_cast<int>(EventName::Initiative); index <= static_cast<int>(EventName::OnCustomEvent); ++index) {
    auto event = static_cast<EventName>(index);
    if (detailed == eventNameText(event)) return event;
  }
  throw std::runtime_error("Unknown detailed event " + std::string(detailed));
}
namespace {
struct Relation {
  const GameState& snapshot;
  EntityArea callerArea;
  int callerId;
  std::string_view listen;
  bool player(int who) const { return listen == "all" || callerArea.who == who; }
  bool area(EntityArea area) const {
    if (listen == "myself") return false;  // JS compares callerId with the area object itself.
    if (listen == "sameArea" && callerArea.type == AreaType::Characters)
      return area.type == AreaType::Characters && callerArea.characterId == area.characterId;
    return player(area.who);
  }
  bool entity(int id) const {
    if (listen == "myself") return callerId == id;
    return area(getEntityArea(snapshot, id));
  }
};
}  // namespace
bool detailedEventFilter(std::string_view name, std::string_view listen, SkillContext& c) {
  if (listen != "myself" && listen != "sameArea" && listen != "samePlayer" && listen != "all") throw std::runtime_error("Unknown listenTo");
  auto& event = c.e<EventArg>();
  Relation relation{*event.onTimeState, c.entityArea(c.self().id()), c.self().id(), listen};
  auto player = [&] { return relation.player(c.e<PlayerEventArg>().who); };
  if (name == "adventure") return relation.player(c.e<EntityEventArg>().who);
  if (name == "battleBegin" || name == "roundBegin" || name == "roundEnd" || name == "actionPhase" || name == "endPhase") return true;
  if (name == "roll" || name == "addDice" || name == "deductElementDice" || name == "beforeAction" || name == "action" || name == "playCard" || name == "declareEnd" || name == "generateDice" || name == "selectCard") return player();
  if (name == "deductOmniDice" || name == "deductOmniDiceSwitch" || name == "deductOmniDiceCard" || name == "deductAllDiceCard") {
    auto& e = c.e<ModifyActionEventArg>();
    return player() && e.canDeductCost() &&
      (name != "deductOmniDiceSwitch" || e.isSwitchActive()) &&
      ((name != "deductOmniDiceCard" && name != "deductAllDiceCard") || e.isPlayCard());
  }
  if (name == "deductVoidDiceSkill" || name == "deductElementDiceSkill" || name == "deductOmniDiceSkill" || name == "deductOmniDiceTechnique") {
    auto& e = c.e<ModifyActionEventArg>();
    return (name == "deductOmniDiceTechnique" ? e.isUseTechnique() : e.isUseCharacterSkill()) &&
      relation.entity(e.action().skill->caller.id()) &&
      (name == "deductVoidDiceSkill" ? e.canDeductVoidCost() : name == "deductElementDiceSkill" || e.canDeductCost());
  }
  if (name == "beforeFastSwitch") { auto& e = c.e<ModifyActionEventArg>(); return player() && e.isSwitchActive() && !e.isFast(); }
  if (name == "modifySkillDamageType" || name == "increaseDamage" || name == "increaseSkillDamage" || name == "increaseTechniqueDamage" || name == "multiplySkillDamage" || name == "increaseDamaged" || name == "multiplyDamaged" || name == "decreaseDamaged") {
    auto& e = c.e<DamageOrHealEventArg>();
    if (e.type() == damage::Piercing) return false;
    if (name == "increaseDamaged" || name == "multiplyDamaged" || name == "decreaseDamaged") return relation.entity(e.target()->id) && (name != "decreaseDamaged" || e.value() > 0);
    if (!relation.entity(e.source().id())) return false;
    if (name == "increaseDamage") return getEntityArea(relation.snapshot, e.source().id()).who != getEntityArea(relation.snapshot, e.target()->id).who;
    if (e.damageInfo().fromReaction != reaction::None) return false;
    return name == "increaseTechniqueDamage" ? e.via()->definition->skillType == SkillType::Technique : e.source().isCharacter();
  }
  if (name == "cancelHealed" || name == "decreaseHealed") { auto& e = c.e<ModifyHealEventArg>(); return relation.entity(e.target()->id) && e.modifiable() && !e.cancelled(); }
  if (name == "beforeDefeated") { auto& e = c.e<ZeroHealthEventArg>(); return relation.entity(e.target()->id) && !e.immuneSkill; }
  if (name == "replaceActionBySkill") {
    auto active = activeCharacter(*event.onTimeState->players[c.e<PlayerEventArg>().who]);
    return relation.entity(active->id) && !isSkillDisabled(*active);
  }
  if (name == "beforeSkill" || name == "beforeTechnique" || name == "useSkill" || name == "useTechnique" || name == "useSkillOrTechnique") {
    auto& e = c.e<UseSkillEventArg>();
    if (!relation.area(e.callerArea) || !e.skill->definition->initiativeSkillConfig || e.skill->definition->initiativeSkillConfig->omitEvents) return false;
    if (name == "beforeTechnique" || name == "useTechnique") return e.isSkillType(SkillType::Technique);
    return isCharacterInitiativeSkill(*e.skill, name == "useSkillOrTechnique");
  }
  if (name == "switchActive") { auto& e = c.e<SwitchActiveEventArg>(); return (e.switchInfo.from && relation.entity(e.switchInfo.from->id)) || relation.entity(e.switchInfo.to->id); }
  if (name == "drawCard" || name == "handCardInserted" || name == "selfHandCardInserted") {
    auto& e = c.e<HandCardInsertedEventArg>();
    if (name == "selfHandCardInserted") return relation.callerId == e.card->id && relation.callerArea.type != AreaType::Pile;
    auto area = getEntityArea(c.state(), e.card->id);
    if (!player()) return false;
    if (name == "drawCard") return (e.reason == HandInsertReason::Draw || e.reason == HandInsertReason::Switch) && (area.type == AreaType::Hands || e.overflowed);
    return area.who == e.who && !e.overflowed;
  }
  if (name == "discard" || name == "discardOrTuneCard" || name == "selfDiscard" || name == "entityDispose" || name == "selfDispose") {
    auto& e = c.e<DisposeEventArg>();
    if (name == "selfDispose") return e.entity.id() == relation.callerId;
    if (name == "selfDiscard") return e.entity.id() == relation.callerId && e.isDiscard();
    if (name == "entityDispose") return !e.isDiscardOrTuning() && relation.entity(e.entity.id());
    return relation.player(e.who) && (name == "discard" ? e.isDiscard() : e.isDiscardOrTuning());
  }
  if (name == "dealDamage" || name == "skillDamage" || name == "damaged" || name == "healed" || name == "damagedOrHealed" || name == "defeated") {
    auto& e = c.e<DamageOrHealEventArg>();
    if (name == "defeated") return relation.entity(e.target()->id) && e.damageInfo().causeDefeated;
    if (name == "healed") return e.isDamageTypeHeal() && relation.entity(e.target()->id);
    if (name == "damagedOrHealed") return relation.entity(e.target()->id);
    if (!e.isDamageTypeDamage()) return false;
    if (name == "damaged") return relation.entity(e.target()->id);
    return relation.entity(e.source().id()) && (name != "skillDamage" || (e.source().isCharacter() && e.damageInfo().fromReaction == reaction::None));
  }
  if (name == "modifyReaction" || name == "reaction" || name == "dealReaction") { auto& e = c.e<ReactionEventArg>(); return relation.entity(name == "dealReaction" ? e.reactionCaller().id() : e.target()->id); }
  if (name == "selfEnter") return c.e<EntityEventArg>().entity.id() == relation.callerId;
  if (name == "entityEnter" || name == "customEvent") return relation.entity(c.e<EntityEventArg>().entity.id());
  if (name == "revive") return relation.entity(c.e<CharacterEventArg>().character->id);
  if (name == "transformDefinition") return relation.entity(c.e<TransformDefinitionEventArg>().entity.id());
  if (name == "cancelConsumeNightsoul" || name == "consumeNightsoul" || name == "gainNightsoul" || name == "consumeUsage" || name == "gainUsage" || name == "consumeEffect" || name == "gainEffect") {
    auto& e = c.e<VariableEventArg>();
    const char* variable = name.ends_with("Nightsoul") ? "nightsoul" : name.ends_with("Usage") ? "usage" : "effect";
    bool increase = name.starts_with("gain");
    return e.info.varName == intern(variable) && relation.area(e.area) && e.info.increase == increase && (name != "cancelConsumeNightsoul" || !e.info.cancelled);
  }
  throw std::runtime_error("Missing detailed event filter " + std::string(name));
}
}  // namespace gitcg
