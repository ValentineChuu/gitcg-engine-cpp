#include "core/definitions/event_name.h"

namespace gitcg {

const char* eventNameText(EventName name) {
  switch (name) {
    case EventName::Initiative: return "initiative";
    case EventName::OnBattleBegin: return "onBattleBegin";
    case EventName::OnRoundBegin: return "onRoundBegin";
    case EventName::OnRoundEnd: return "onRoundEnd";
    case EventName::ModifyRoll: return "modifyRoll";
    case EventName::OnActionPhase: return "onActionPhase";
    case EventName::OnEndPhase: return "onEndPhase";
    case EventName::ReplaceAction: return "replaceAction";
    case EventName::OnBeforeAction: return "onBeforeAction";
    case EventName::ModifyAction0: return "modifyAction0";
    case EventName::ModifyAction1: return "modifyAction1";
    case EventName::ModifyAction2: return "modifyAction2";
    case EventName::ModifyAction3: return "modifyAction3";
    case EventName::ModifyAction4: return "modifyAction4";
    case EventName::OnAction: return "onAction";
    case EventName::OnBeforeUseSkill: return "onBeforeUseSkill";
    case EventName::OnUseSkill: return "onUseSkill";
    case EventName::OnBeforePlayCard: return "onBeforePlayCard";
    case EventName::OnPlayCard: return "onPlayCard";
    case EventName::OnDeclareEnd: return "onDeclareEnd";
    case EventName::OnSwitchActive: return "onSwitchActive";
    case EventName::OnHandCardInserted: return "onHandCardInserted";
    case EventName::OnReaction: return "onReaction";
    case EventName::OnTransformDefinition: return "onTransformDefinition";
    case EventName::OnGenerateDice: return "onGenerateDice";
    case EventName::ModifyChangeVariable: return "modifyChangeVariable";
    case EventName::OnChangeVariable: return "onChangeVariable";
    case EventName::OnSelectCard: return "onSelectCard";
    case EventName::OnAdventure: return "onAdventure";
    case EventName::ModifyDamage0: return "modifyDamage0";
    case EventName::ModifyDamage1: return "modifyDamage1";
    case EventName::ModifyDamage2: return "modifyDamage2";
    case EventName::ModifyDamage3: return "modifyDamage3";
    case EventName::ModifyHeal0: return "modifyHeal0";
    case EventName::ModifyHeal1: return "modifyHeal1";
    case EventName::ModifyReaction: return "modifyReaction";
    case EventName::OnDamageOrHeal: return "onDamageOrHeal";
    case EventName::OnEnter: return "onEnter";
    case EventName::OnDispose: return "onDispose";
    case EventName::ModifyZeroHealth: return "modifyZeroHealth";
    case EventName::OnRevive: return "onRevive";
    case EventName::OnCustomEvent: return "onCustomEvent";
    case EventName::RequestSwitchHands: return "requestSwitchHands";
    case EventName::RequestSelectCard: return "requestSelectCard";
    case EventName::RequestAdventure: return "requestAdventure";
    case EventName::RequestReroll: return "requestReroll";
    case EventName::RequestUseSkill: return "requestUseSkill";
    case EventName::RequestPlayCard: return "requestPlayCard";
    case EventName::RequestTriggerEndPhaseSkill: return "requestTriggerEndPhaseSkill";
  }
  return "unknown";
}

}  // namespace gitcg
