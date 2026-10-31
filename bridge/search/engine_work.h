// Counts corresponding to reference/bridge/engine_work.mjs, emitted only when requested by the environment.
#pragma once
#include "core/json/json_value.h"
namespace gitcg {
struct SearchWork { int64_t searches = 0, plays = 0, overBudget = 0; };
struct EngineWork { int64_t decisions = 0, endOfRoundCopies = 0; SearchWork replies, followUps; };
extern EngineWork engineWork;
void reportEngineWork();
}
