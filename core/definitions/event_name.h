// Core event and request names (core/src/base/skill.ts EVENT_MAP and REQUEST_MAP), plus "initiative" for skills
// a player uses as an action.
#pragma once

namespace gitcg {

enum class EventName {
  Initiative,
  OnBattleBegin,
  OnRoundBegin,
  OnRoundEnd,
  ModifyRoll,
  OnActionPhase,
  OnEndPhase,
  ReplaceAction,
  OnBeforeAction,
  ModifyAction0,
  ModifyAction1,
  ModifyAction2,
  ModifyAction3,
  ModifyAction4,
  OnAction,
  OnBeforeUseSkill,
  OnUseSkill,
  OnBeforePlayCard,
  OnPlayCard,
  OnDeclareEnd,
  OnSwitchActive,
  OnHandCardInserted,
  OnReaction,
  OnTransformDefinition,
  OnGenerateDice,
  ModifyChangeVariable,
  OnChangeVariable,
  OnSelectCard,
  OnAdventure,
  ModifyDamage0,
  ModifyDamage1,
  ModifyDamage2,
  ModifyDamage3,
  ModifyHeal0,
  ModifyHeal1,
  ModifyReaction,
  OnDamageOrHeal,
  OnEnter,
  OnDispose,
  ModifyZeroHealth,
  OnRevive,
  OnCustomEvent,
  // Requests (handled by the skill executor, never by skills).
  RequestSwitchHands,
  RequestSelectCard,
  RequestAdventure,
  RequestReroll,
  RequestUseSkill,
  RequestPlayCard,
  RequestTriggerEndPhaseSkill,
};

const char* eventNameText(EventName name);

}  // namespace gitcg
