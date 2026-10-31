#include "bridge/search/engine_work.h"
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include "core/json/json_text.h"
namespace gitcg {
EngineWork engineWork;
void reportEngineWork() {
  const char* enabled = std::getenv("AUTOGITCG_ENGINE_WORK");
  if (!enabled || std::string_view(enabled) != "1") return;
  auto search = [](const SearchWork& work) {
    JsonValue result = JsonValue::object(); result["searches"] = work.searches; result["plays"] = work.plays;
    result["overBudget"] = work.overBudget; return result;
  };
  JsonValue result = JsonValue::object(); result["decisions"] = engineWork.decisions;
  result["replies"] = search(engineWork.replies); result["followUps"] = search(engineWork.followUps);
  JsonValue end = JsonValue::object(); end["copies"] = engineWork.endOfRoundCopies; result["endOfRound"] = std::move(end);
  std::fprintf(stderr, "ENGINE_WORK %s\n", writeJson(result).c_str());
}
}
