// SkillContext: queries (evaluated for the calling entity's player on the context's current state).

#include "core/skills/skill_context.h"
#include "query/builder.h"
#include "query/runtime.h"

namespace gitcg {

std::vector<AnyHandle> SkillContext::queryAll(const QueryExpression& query) {
  return handlesOf(runQuery(state(), selfWho(), query));
}

std::optional<AnyHandle> SkillContext::query(const QueryExpression& query) {
  std::vector<AnyHandle> results = queryAll(query);
  if (results.empty()) return std::nullopt;
  return results.front();
}

}  // namespace gitcg
