// The $ builder. Getters become calls; C++ keywords not and union use not_ and union_ (AGPL-3.0).
#pragma once

#include <memory>
#include <vector>

#include "query/expression.h"
#include "query/runtime.h"

namespace gitcg::query {
struct BuilderData;
class Macros;
struct Keys { const char* diceCost = diceCostKey; const char* inInitialPile = inInitialPileKey; };

class Query {
 public:
  Query();
  static Query root();
  Query any() const;
  QueryExpression toExpression() const;
  QueryExpression toExpressionUnordered() const;
  operator QueryExpression() const { return toExpression(); }
  Keys keys() const { return {}; }
  Macros macros() const;

  Query my() const; Query opp() const;
  Query onStage() const; Query offStage() const;
  Query character() const; Query combatStatus() const; Query summon() const; Query support() const;
  Query attachment() const; Query hand() const; Query pile() const;
  Query vCharacter() const; Query vHand() const; Query vPile() const;
  Query typeEquipment() const; Query typeSupport() const; Query typeStatus() const; Query typeEventCard() const;
  Query equipped() const; Query equipment() const; Query status() const; Query eventCard() const;
  Query active() const; Query prev() const; Query next() const; Query standby() const;
  Query onlyDefeated() const; Query includesDefeated() const; Query notInitial() const;
  Query id(double value) const; Query def(double value) const;
  Query tag(const std::vector<std::string>& tags = {}) const;
  template <typename... Tags> Query tag(const std::string& first, const Tags&... rest) const {
    return tag(std::vector<std::string>{first, rest...});
  }
  Query tagOf(const std::string& category, const Query& base) const;
  Query var(JsonValue key, double value) const;
  Query var(JsonValue key, const std::string& op, JsonValue value) const;
  Query var(JsonValue key, std::function<double(double)> predicate) const;
  Query var(QueryFunction predicate) const;
  Query cost(double value) const; Query cost(const std::string& op, double value) const;
  Query cost(std::function<double(double)> predicate) const;

  Query has(const Query& operand) const; Query at(const Query& operand) const;
  Query with(const Query& operand) const; Query on(const Query& operand) const;
  Query not_(const Query& operand) const; Query recentOppFrom(const Query& operand) const;
  Query has() const; Query at() const; Query with() const; Query on() const;
  Query not_() const; Query recentOppFrom() const;
  Query exclude(const Query& right) const; Query orElse(const Query& right) const;
  Query union_(const Query& right) const; Query intersection(const Query& right) const;
  Query union_(const std::vector<Query>& operands = {}) const;
  Query intersection(const std::vector<Query>& operands = {}) const;
  template <typename... Queries> Query union_(const Query& first, const Query& second, const Queries&... rest) const {
    return union_(std::vector<Query>{first, second, rest...});
  }
  template <typename... Queries> Query intersection(const Query& first, const Query& second, const Queries&... rest) const {
    return intersection(std::vector<Query>{first, second, rest...});
  }
  Query orderBy(JsonValue variable) const;
  Query orderBy(JsonValue left, const std::string& op, JsonValue right) const;
  Query orderByFn(QueryFunction function) const;
  Query orderByRaw(const std::vector<JsonValue>& specifications = {}) const;
  template <typename... Specifications> Query orderByRaw(JsonValue first, const Specifications&... rest) const {
    return orderByRaw(std::vector<JsonValue>{first, rest...});
  }
  Query limit(double count) const;

 private:
  explicit Query(std::shared_ptr<BuilderData> data) : data_(std::move(data)) {}
  Query constraint(JsonValue expression) const;
  Query constraint(const QueryExpression& expression) const;
  Query defeated(int mode) const;
  Query unary(const std::string& op) const;
  Query relation(const std::string& op, const Query& operand) const;
  Query composite(const std::string& op, const std::vector<Query>& operands) const;
  Query ordered() const;
  std::shared_ptr<BuilderData> data_;
};

inline Query dollar() { return Query::root(); }
}  // namespace gitcg::query

namespace gitcg {
inline std::vector<AnyState> runQuery(const GameState& state, int who, const query::Query& query) {
  return runQuery(state, who, query.toExpression());
}
}  // namespace gitcg
