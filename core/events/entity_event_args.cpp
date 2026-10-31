#include "core/events/entity_event_args.h"

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"
#include "core/rules/costs.h"
#include "core/rules/entity_lookup.h"
#include "core/rules/reaction_table.h"

namespace gitcg {

EntityEventArg::EntityEventArg(GameStatePointer state, AnyState subject)
    : EventArg(std::move(state)), entity(std::move(subject)), area(getEntityArea(*onTimeState, entity.id())), who(area.who) {}

int DisposeEventArg::diceCost() const {
  return diceCostSizeOfCard(onTimeState, *disposedEntity());
}

CharacterEventArg::CharacterEventArg(GameStatePointer state, CharacterPointer subject)
    : EventArg(std::move(state)), character(std::move(subject)), who(getEntityArea(*onTimeState, character->id).who) {}

ReactionEventArg::ReactionEventArg(GameStatePointer state, ReactionInfo info)
    : CharacterEventArg(std::move(state), info.target),
      reactionInfo(std::move(info)),
      callerArea_(getEntityArea(*onTimeState, reactionInfo.via->caller.id())) {}

bool ReactionEventArg::relatedTo(DamageType element) const {
  return reactionInvolves(reactionInfo.type, element);
}

ModifyReactionEventArg::ModifyReactionEventArg(GameStatePointer state, ReactionInfo info)
    : ReactionEventArg(std::move(state), std::move(info)), whereIsMine(viaWho() == who) {}

int DefinitionReference::id() const {
  return character ? character->id : entity->id;
}

TransformDefinitionEventArg::TransformDefinitionEventArg(GameStatePointer state, AnyState subject, DefinitionReference transformedTo)
    : EventArg(std::move(state)), entity(std::move(subject)), who(getEntityArea(*onTimeState, entity.id()).who), newDefinition(transformedTo) {
  if (entity.isCharacter()) {
    oldDefinition.character = entity.character()->definition;
  } else {
    oldDefinition.entity = entity.entity()->definition;
  }
}

}  // namespace gitcg
