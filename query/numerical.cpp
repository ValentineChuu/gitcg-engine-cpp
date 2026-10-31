// Numerical S-expression interpreter; operators and evaluation order follow query/runtime.ts (AGPL-3.0).
#include "query/numerical.h"

#include <cmath>
#include <limits>
#include <stdexcept>

#include "query/function.h"

namespace gitcg::query {
bool truthy(double value) { return value != 0 && !std::isnan(value); }

double evaluateNumerical(const JsonValue& expression, const QueryVariables& variables) {
  if (expression.isNumber()) return expression.asNumber();
  if (expression.isString()) return variables(expression.asString());
  const auto& args = expression.asArray();
  if (args.empty()) throw std::runtime_error("Empty numerical expression");
  const auto& op = args[0].asString();
  const size_t count = args.size() - 1;
  auto require = [&](size_t expected) {
    if (count != expected) throw std::runtime_error("Invalid argument count for " + op);
  };
  auto value = [&](size_t index) { return evaluateNumerical(args.at(index), variables); };
  if (op == "special:diceCost") return variables(diceCostKey);
  if (op == "special:inInitialPile") return variables(inInitialPileKey);
  if (op == "and" || op == "or") {
    for (size_t i = 1; i < args.size(); ++i) {
      if (truthy(value(i)) == (op == "or")) return op == "or";
    }
    return op == "and";
  }
  if (op == "not") { require(1); return !truthy(value(1)); }
  if (op == "+" || op == "*" || op == "min" || op == "max") {
    double result = op == "+" ? 0 : op == "*" ? 1 : op == "min"
        ? std::numeric_limits<double>::infinity() : -std::numeric_limits<double>::infinity();
    for (size_t i = 1; i < args.size(); ++i) {
      double next = value(i);
      if (op == "+") result += next;
      else if (op == "*") result *= next;
      else if (std::isnan(result) || std::isnan(next)) result = std::numeric_limits<double>::quiet_NaN();
      else if (op == "min") {
        if (next < result || (next == 0 && result == 0 && std::signbit(next))) result = next;
      } else if (next > result || (next == 0 && result == 0 && !std::signbit(next))) result = next;
    }
    return result;
  }
  if (op == "-" || op == "/") {
    if (count != 1 && count != 2) throw std::runtime_error("Invalid argument count for " + op);
    double left = value(1);
    if (count == 1) return op == "-" ? -left : 1 / left;
    double right = value(2);
    return op == "-" ? left - right : left / right;
  }
  require(2);
  double left = value(1), right = value(2);
  if (op == "%") return std::fmod(left, right);
  if (op == "=") return left == right;
  if (op == "!=") return left != right;
  if (op == "<") return left < right;
  if (op == "<=") return left <= right;
  if (op == ">") return left > right;
  if (op == ">=") return left >= right;
  throw std::runtime_error("Unknown numerical operator " + op);
}

double evaluateSpecification(const JsonValue& spec, const QueryVariables& variables, const QueryExpression& query) {
  const auto& args = spec.asArray();
  if (args.at(0).asString() == "expr") return evaluateNumerical(args.at(1), variables);
  if (args.at(0).asString() == "fn") {
    const auto& code = args.at(1).asString();
    auto found = query.functions.find(code);
    if (found != query.functions.end()) return found->second(variables);
    return evaluateFunction(code, variables);
  }
  throw std::runtime_error("Unknown variable/order specification");
}
}  // namespace gitcg::query
