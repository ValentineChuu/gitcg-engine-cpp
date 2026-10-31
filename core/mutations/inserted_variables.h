// Variables of an entity entering an area (core/src/utils.ts getInsertedStateVariables and shouldEnterOverride).
#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "core/definitions/entity_definition.h"
#include "core/mutations/mutator.h"
#include "core/state/any_state.h"

namespace gitcg {

// The entity of the same definition already in the area, which a new one overrides (never for supports).
EntityPointer findOverriddenEntity(const std::vector<AnyState>& existing, const EntityDefinition& incoming);

// For an overridden entity, the variables to rewrite (by recreate behavior); otherwise every variable of the new
// entity. `incomingVariables`: a moving entity's own variables, else the options' overrides.
std::vector<std::pair<Name, double>> insertedStateVariables(const GameState& state, const EntityPointer& oldState,
                                                            const EntityDefinition& definition,
                                                            const std::vector<std::pair<Name, double>>& incomingVariables,
                                                            bool modifyOverriddenVariablesOnly);

}  // namespace gitcg
