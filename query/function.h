// Portable function specifications for arithmetic predicates; translated C++ functions use native callbacks.
#pragma once
#include "query/expression.h"
namespace gitcg::query {
double evaluateFunction(const std::string& code, const QueryVariables& variables);
double evaluateScalarFunction(const std::string& code, double value);
}  // namespace gitcg::query
