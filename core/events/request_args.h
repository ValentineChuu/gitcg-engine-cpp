// Requests a skill emits for the executor to carry out after it (core/src/base/skill.ts REQUEST_MAP and
// SelectCardInfo), and the select-card event argument.
#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "core/events/event_arg.h"

namespace gitcg {

struct EntityDefinition;

// Which targets a requested card play uses: a fixed list, or a rule.
struct PlayCardTarget {
  enum class Rule { List, SkipIfRequired, First, Random };
  Rule rule = Rule::SkipIfRequired;
  std::vector<AnyState> targets;  // List only
};

struct SelectCardInfo {
  enum class Kind { CreateHandCard, CreateEntity, RequestPlayCard };
  Kind kind = Kind::CreateHandCard;
  std::vector<const EntityDefinition*> cards;
  PlayCardTarget target;  // RequestPlayCard only
};

class RequestArg : public SkillArgument {
 public:
  RequestArg(std::shared_ptr<const SkillInfo> requestedBy, int player) : via(std::move(requestedBy)), who(player) {}
  std::shared_ptr<const SkillInfo> via;
  int who;
};

class RerollRequestArg : public RequestArg {
 public:
  RerollRequestArg(std::shared_ptr<const SkillInfo> requestedBy, int player, int rerollTimes)
      : RequestArg(std::move(requestedBy), player), times(rerollTimes) {}
  int times;
};

class SelectCardRequestArg : public RequestArg {
 public:
  SelectCardRequestArg(std::shared_ptr<const SkillInfo> requestedBy, int player, SelectCardInfo selectInfo)
      : RequestArg(std::move(requestedBy), player), info(std::move(selectInfo)) {}
  SelectCardInfo info;
};

class UseSkillRequestArg : public RequestArg {
 public:
  UseSkillRequestArg(std::shared_ptr<const SkillInfo> requestedBy, int player, double skillId, bool prepared)
      : RequestArg(std::move(requestedBy), player), requestingSkillId(skillId), asPrepared(prepared) {}
  double requestingSkillId;
  bool asPrepared;
};

class PlayCardRequestArg : public RequestArg {
 public:
  PlayCardRequestArg(std::shared_ptr<const SkillInfo> requestedBy, int player, EntityPointer playedCard, PlayCardTarget playTarget,
                     bool selected)
      : RequestArg(std::move(requestedBy), player), card(std::move(playedCard)), target(std::move(playTarget)), viaSelect(selected) {}
  EntityPointer card;
  PlayCardTarget target;
  bool viaSelect;
};

class TriggerEndPhaseSkillRequestArg : public RequestArg {
 public:
  TriggerEndPhaseSkillRequestArg(std::shared_ptr<const SkillInfo> requestedBy, int player, EntityPointer requested)
      : RequestArg(std::move(requestedBy), player), requestedEntity(std::move(requested)) {}
  EntityPointer requestedEntity;
};

// requestSwitchHands and requestAdventure need only `via` and `who`: plain RequestArg.

class SelectCardEventArg : public PlayerEventArg {
 public:
  SelectCardEventArg(GameStatePointer state, int player, SelectCardInfo selectInfo)
      : PlayerEventArg(std::move(state), player), info(std::move(selectInfo)) {}
  SelectCardInfo info;
};

}  // namespace gitcg
