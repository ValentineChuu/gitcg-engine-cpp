// JavaScript numeric expressions: missing variables are NaN, and NaN is false (AGPL-3.0).
#pragma once

#include "query/expression.h"

namespace gitcg::query {
bool truthy(double value);
double evaluateNumerical(const JsonValue& expression, const QueryVariables& variables);
double evaluateSpecification(const JsonValue& specification, const QueryVariables& variables,
                             const QueryExpression& query);
}  // namespace gitcg::query
