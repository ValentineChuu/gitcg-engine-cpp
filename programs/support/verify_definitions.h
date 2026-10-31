// Exhaustive checks of the static dump's fields against the loaded C++ records.
#pragma once
#include "programs/support/load_definitions.h"
namespace gitcg::testing {
void verifyDefinitions(const LoadedDefinitions& loaded);
}  // namespace gitcg::testing
