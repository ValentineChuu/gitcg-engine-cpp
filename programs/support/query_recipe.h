// Replays method calls independently in C++ so differential tests check the builder as well as the runtime.
#pragma once
#include "query/macro.h"
namespace gitcg::testing {
query::Query buildQueryRecipe(const JsonValue& recipe);
bool recipeHasFunction(const JsonValue& recipe);
}  // namespace gitcg::testing
