// SkillContext: requests carried out by the executor after the skill (reroll, card switch, using skills, playing
// and selecting cards, adventures).

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"
#include "core/definitions/game_data.h"
#include "core/rules/entity_lookup.h"
#include "core/skills/skill_context.h"
#include "core/values/engine_errors.h"
#include "query/builder.h"

namespace gitcg {

void SkillContext::switchCards() {
  emitEvent(EventName::RequestSwitchHands, std::make_shared<RequestArg>(sharedSkillInfo(), selfWho()));
}

void SkillContext::rerollDice(int times) {
  emitEvent(EventName::RequestReroll, std::make_shared<RerollRequestArg>(sharedSkillInfo(), selfWho(), times));
}

void SkillContext::triggerEndPhaseSkill(const EntityHandle& target) {
  emitEvent(EventName::RequestTriggerEndPhaseSkill,
            std::make_shared<TriggerEndPhaseSkillRequestArg>(sharedSkillInfo(), selfWho(), target.latest()));
}

void SkillContext::useSkill(double skillId, bool asPrepared) {
  emitEvent(EventName::RequestUseSkill, std::make_shared<UseSkillRequestArg>(sharedSkillInfo(), selfWho(), skillId, asPrepared));
}

void SkillContext::useNormalSkill(bool asPrepared) {
  std::optional<AnyHandle> active = query(query::dollar().my().active());
  if (!active) return;
  for (const SkillDefinition* skill : active->asCharacter().definition().skills) {
    if (skill->skillType == SkillType::Normal) {
      useSkill(skill->id, asPrepared);
      return;
    }
  }
}

void SkillContext::playCard(const EntityHandle& card, PlayCardTarget target) {
  EntityPointer cardState = card.latest();
  if (card.area().type != AreaType::Hands) throw GameDataError("Cannot play a card that is not in hands");
  if (target.rule == PlayCardTarget::Rule::List) {
    for (AnyState& state : target.targets) state = getEntityById(this->state(), state.id());
  }
  emitEvent(EventName::RequestPlayCard, std::make_shared<PlayCardRequestArg>(sharedSkillInfo(), selfWho(), cardState, target, false));
}

namespace {

std::vector<const EntityDefinition*> definitionsOf(const GameState& state, const std::vector<int>& ids) {
  std::vector<const EntityDefinition*> definitions;
  for (int id : ids) {
    const EntityDefinition* definition = state.data->entities.find(id);
    if (!definition) throw GameDataError("Unknown card definition id " + std::to_string(id));
    definitions.push_back(definition);
  }
  return definitions;
}

}  // namespace

void SkillContext::selectAndSummon(const std::vector<int>& summonIds) {
  SelectCardInfo info{SelectCardInfo::Kind::CreateEntity, definitionsOf(state(), summonIds), {}};
  emitEvent(EventName::RequestSelectCard, std::make_shared<SelectCardRequestArg>(sharedSkillInfo(), selfWho(), info));
}

void SkillContext::selectAndCreateHandCard(const std::vector<int>& cardIds) {
  SelectCardInfo info{SelectCardInfo::Kind::CreateHandCard, definitionsOf(state(), cardIds), {}};
  emitEvent(EventName::RequestSelectCard, std::make_shared<SelectCardRequestArg>(sharedSkillInfo(), selfWho(), info));
}

void SkillContext::selectAndPlay(const std::vector<int>& cardIds, PlayCardTarget target) {
  if (target.rule == PlayCardTarget::Rule::List) {
    for (AnyState& state : target.targets) state = getEntityById(this->state(), state.id());
  }
  SelectCardInfo info{SelectCardInfo::Kind::RequestPlayCard, definitionsOf(state(), cardIds), target};
  emitEvent(EventName::RequestSelectCard, std::make_shared<SelectCardRequestArg>(sharedSkillInfo(), selfWho(), info));
}

void SkillContext::adventure() {
  emitEvent(EventName::RequestAdventure, std::make_shared<RequestArg>(sharedSkillInfo(), selfWho()));
}

void SkillContext::finishAdventure() {
  static const Name ADVENTURE_SPOT = intern("adventureSpot");
  const EntityDefinition& definition = selfEntity().definition();
  if (definition.type != EntityType::Support || !definition.hasTag(ADVENTURE_SPOT)) {
    throw GameDataError("Only support card with adventureSpot tag can call .finishAdventure()");
  }
  const int ADVENTURE_COMPLETE_ID = 171;
  if (state().data->entities.find(ADVENTURE_COMPLETE_ID)) combatStatus(ADVENTURE_COMPLETE_ID);
  dispose();
}

}  // namespace gitcg
