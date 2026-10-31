// Mutable primary chains and ordered snapshots mirror the JavaScript builder (AGPL-3.0).
#include "query/builder.h"

#include <atomic>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "query/numerical.h"

namespace gitcg::query {
struct BuilderData {
  enum class Kind { Root, Primary, Composite, Ordered } kind = Kind::Primary;
  int defeated = 0;  // default ignore, 1 only, 2 include
  std::string leading, compositeOperator;
  std::vector<QueryExpression> constraints;
  std::vector<Query> operands;
  QueryExpression unordered;
  std::vector<JsonValue> specifications;
  double limit = std::numeric_limits<double>::infinity();
};
namespace {
void merge(QueryExpression& target, const QueryExpression& source) {
  target.functions.insert(source.functions.begin(), source.functions.end());
}
QueryExpression functionSpecification(QueryFunction function) {
  static std::atomic<unsigned long long> next{0};
  std::string code = "native:" + std::to_string(next++);
  QueryExpression result(list({"fn", code}));
  result.functions.emplace(code, std::move(function));
  return result;
}
}  // namespace
Query::Query() : data_(std::make_shared<BuilderData>()) {}
Query Query::root() { Query result; result.data_->kind = BuilderData::Kind::Root; return result; }
Query Query::any() const { return data_->kind == BuilderData::Kind::Root ? Query() : *this; }
QueryExpression Query::toExpressionUnordered() const {
  if (data_->kind == BuilderData::Kind::Ordered || data_->kind == BuilderData::Kind::Root) {
    throw std::runtime_error("Expected an unordered query");
  }
  QueryExpression result;
  if (data_->kind == BuilderData::Kind::Composite) {
    result.expression = list({data_->compositeOperator});
    for (const auto& operand : data_->operands) {
      auto expression = operand.toExpressionUnordered();
      result.expression.push(expression.expression);
      merge(result, expression);
    }
    return result;
  }
  JsonValue constraints = JsonValue::array();
  auto append = [&](const JsonValue& value) {
    // The two defeated constraints are shared constants in JS; Set removes their repeated references.
    const auto& args = value.asArray();
    if (args.size() == 2 && args[0].isString() && args[0].asString() == "defeated") {
      for (const auto& existing : constraints.asArray()) if (existing == value) return;
    }
    constraints.push(value);
  };
  const size_t finalCount = data_->constraints.size() + (data_->defeated != 2);
  if (data_->defeated != 2) append(list({"defeated", data_->defeated == 1 ? "only" : "ignore"}));
  for (const auto& expression : data_->constraints) {
    merge(result, expression);
    const auto& items = expression.expression.asArray();
    if (!items.empty() && items[0].isString() && items[0].asString() == "intersection") {
      for (size_t i = 1; i < items.size(); ++i) append(items[i]);
    } else append(expression.expression);
  }
  if (finalCount == 1) result.expression = constraints.asArray()[0];
  else {
    result.expression = list({"intersection"});
    for (const auto& value : constraints.asArray()) result.expression.push(value);
  }
  if (!data_->leading.empty()) result.expression = list({data_->leading, result.expression});
  return result;
}
QueryExpression Query::toExpression() const {
  if (data_->kind != BuilderData::Kind::Ordered) return toExpressionUnordered();
  QueryExpression result = data_->unordered;
  result.expression = list({"orderBy", data_->unordered.expression, JsonValue(data_->specifications), data_->limit});
  return result;
}
Query Query::constraint(JsonValue expression) const { return constraint(QueryExpression(std::move(expression))); }
Query Query::constraint(const QueryExpression& expression) const {
  Query result = any();
  if (result.data_->kind != BuilderData::Kind::Primary) throw std::runtime_error("Primary method on a composite/ordered query");
  result.data_->constraints.push_back(expression);
  return result;
}
#define SIMPLE_METHOD(name, ...) Query Query::name() const { return constraint(list({__VA_ARGS__})); }
SIMPLE_METHOD(my, "who", "my")
SIMPLE_METHOD(opp, "who", "opp")
SIMPLE_METHOD(onStage, "onStage")
SIMPLE_METHOD(offStage, "offStage")
SIMPLE_METHOD(character, "area", "characters", "true")
SIMPLE_METHOD(combatStatus, "area", "combatStatuses", "true")
SIMPLE_METHOD(summon, "area", "summons", "true")
SIMPLE_METHOD(support, "area", "supports", "true")
SIMPLE_METHOD(hand, "area", "hands", "true")
SIMPLE_METHOD(pile, "area", "pile", "true")
SIMPLE_METHOD(vCharacter, "area", "characters", "false")
SIMPLE_METHOD(vHand, "area", "hands", "false")
SIMPLE_METHOD(vPile, "area", "pile", "false")
SIMPLE_METHOD(attachment, "type", "attachment")
SIMPLE_METHOD(typeEquipment, "type", "equipment")
SIMPLE_METHOD(typeSupport, "type", "support")
SIMPLE_METHOD(typeStatus, "type", "status")
SIMPLE_METHOD(typeEventCard, "type", "eventCard")
SIMPLE_METHOD(active, "position", "active")
SIMPLE_METHOD(prev, "position", "prev")
SIMPLE_METHOD(next, "position", "next")
SIMPLE_METHOD(standby, "position", "standby")
#undef SIMPLE_METHOD
Query Query::equipment() const { return typeEquipment(); }
Query Query::status() const { return typeStatus(); }
Query Query::eventCard() const { return typeEventCard(); }
Query Query::equipped() const { return onStage().typeEquipment(); }
Query Query::defeated(int mode) const { Query result = any(); result.data_->defeated = mode; return result; }
Query Query::onlyDefeated() const { return defeated(1); }
Query Query::includesDefeated() const { return defeated(2); }
Query Query::notInitial() const { return var(inInitialPileKey, 0); }
Query Query::id(double value) const { return constraint(list({"id", value})); }
Query Query::def(double value) const { return constraint(list({"definition", value})); }
Query Query::tag(const std::vector<std::string>& tags) const {
  Query result = any();
  for (const auto& tag : tags) result = result.constraint(list({"tag", tag}));
  return result;
}
Query Query::tagOf(const std::string& category, const Query& base) const {
  auto expression = base.toExpressionUnordered();
  expression.expression = list({"tagOf", category, expression.expression});
  return constraint(expression);
}
Query Query::var(JsonValue key, double value) const { return var(key, "=", value); }
Query Query::var(JsonValue key, const std::string& op, JsonValue value) const {
  return constraint(list({"variables", list({"expr", list({op, variableExpression(key), variableExpression(value)})})}));
}
Query Query::var(QueryFunction predicate) const {
  auto spec = functionSpecification(std::move(predicate));
  spec.expression = list({"variables", spec.expression});
  return constraint(spec);
}
Query Query::var(JsonValue key, std::function<double(double)> predicate) const {
  auto expression = variableExpression(key);
  return var([expression, predicate](const QueryVariables& variables) {
    double value = evaluateNumerical(expression, variables);
    return std::isnan(value) ? 0.0 : predicate(value);
  });
}
Query Query::cost(double value) const { return var(diceCostKey, value); }
Query Query::cost(const std::string& op, double value) const { return var(diceCostKey, op, value); }
Query Query::cost(std::function<double(double)> predicate) const { return var(diceCostKey, std::move(predicate)); }
Query Query::unary(const std::string& op) const { Query result; result.data_->leading = op; return result; }
Query Query::relation(const std::string& op, const Query& operand) const {
  auto expression = operand.toExpressionUnordered();
  if (data_->kind == BuilderData::Kind::Root) {
    Query result = unary(op);
    result.data_->constraints.push_back(expression);
    return result;
  }
  expression.expression = list({op, expression.expression});
  return constraint(expression);
}
#define RELATION(name, op) \
  Query Query::name(const Query& operand) const { return relation(op, operand); } \
  Query Query::name() const { return unary(op); }
RELATION(has, "has")
RELATION(at, "at")
RELATION(with, "with")
RELATION(on, "on")
RELATION(not_, "not")
RELATION(recentOppFrom, "recentOppFrom")
#undef RELATION
Query Query::composite(const std::string& op, const std::vector<Query>& operands) const {
  Query result;
  result.data_->kind = BuilderData::Kind::Composite;
  result.data_->compositeOperator = op;
  if (data_->kind != BuilderData::Kind::Root) result.data_->operands.push_back(*this);
  result.data_->operands.insert(result.data_->operands.end(), operands.begin(), operands.end());
  return result;
}
Query Query::exclude(const Query& right) const { return composite("exclude", {right}); }
Query Query::orElse(const Query& right) const { return composite("orElse", {right}); }
Query Query::union_(const Query& right) const { return composite("union", {right}); }
Query Query::intersection(const Query& right) const { return composite("intersection", {right}); }
Query Query::union_(const std::vector<Query>& operands) const { return composite("union", operands); }
Query Query::intersection(const std::vector<Query>& operands) const { return composite("intersection", operands); }
Query Query::ordered() const {
  if (data_->kind == BuilderData::Kind::Ordered) return *this;
  Query result;
  result.data_->kind = BuilderData::Kind::Ordered;
  result.data_->unordered = toExpressionUnordered();
  return result;
}
Query Query::orderBy(JsonValue variable) const { return orderByRaw({list({"expr", variableExpression(variable)})}); }
Query Query::orderBy(JsonValue left, const std::string& op, JsonValue right) const {
  return orderByRaw({list({"expr", list({op, variableExpression(left), variableExpression(right)})})});
}
Query Query::orderByRaw(const std::vector<JsonValue>& specs) const {
  Query result = ordered();
  result.data_->specifications.insert(result.data_->specifications.end(), specs.begin(), specs.end());
  return result;
}
Query Query::orderByFn(QueryFunction function) const {
  auto spec = functionSpecification(std::move(function));
  Query result = orderByRaw({spec.expression});
  merge(result.data_->unordered, spec);
  return result;
}
Query Query::limit(double count) const { Query result = ordered(); result.data_->limit = count; return result; }
}  // namespace gitcg::query
