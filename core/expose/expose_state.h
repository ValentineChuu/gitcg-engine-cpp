// One player's view of the game and the request messages, in the core's JSON shape (core/src/io.ts exposeState,
// exposeEntity, exposeCharacter, exposeAction; @gi-tcg/typings createRpcRequest). Field order matches the JS
// object literals and undefined fields are left out, so the JSON text matches Node's.
#pragma once

#include <vector>

#include "core/events/action_info.h"
#include "core/json/json_value.h"
#include "core/state/game_state.h"

namespace gitcg {

JsonValue exposeState(int who, const GameStatePointer& state);
JsonValue exposeAction(const ActionInfo& action);
JsonValue exposeEntityForMutation(const EntityState& entity, bool hide);
JsonValue exposeAttachmentForMutation(const EntityState& attachment);

JsonValue actionRequest(const std::vector<ActionInfo>& actions);
JsonValue chooseActiveRequest(const std::vector<int>& candidateIds);
JsonValue selectCardRequest(const std::vector<int>& candidateDefinitionIds);
JsonValue rerollDiceRequest();
JsonValue switchHandsRequest();

}  // namespace gitcg
