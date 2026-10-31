# C++ queries

This is an AGPL-3.0 port of Piovium core 0.21.0's query builder and runtime.

```cpp
#include "query/query.h"

using namespace gitcg;
auto query = query::dollar().opp().character().def(1208);
std::vector<AnyState> result = runQuery(state, who, query);

auto cheapest = query::dollar().my().hand()
    .orderBy(query::dollar().keys().diceCost).limit(2);
auto injured = query::dollar().my().character().var(
    [](const QueryVariables& v) { return v("health") < v("maxHealth"); });
auto macro = query::dollar().macros().oppMostInjured();
```

Every JavaScript getter becomes a call. All public query methods keep their names except
`not` and `union`, which are C++ keywords and become `not_` and `union_`.
The real methods are singular: `summon()`, `hand()`, `character()`, and so on.
Variadic `tag`, `union_`, `intersection`, and `orderByRaw` also accept vectors.

Primary chains mutate shared builder state, as in JS. Composite operators retain references to
their operands, whereas ordering takes a snapshot. Macro calls return fresh equivalents of the
JS macro expressions. Native function specifications live with the `QueryExpression` and survive
composition. Their `native:...` strings identify C++ callbacks and are not portable JSON functions.

`QueryExpression(JsonValue)` accepts the exact nested arrays returned by `queryToExpression`.
`Query::toExpression()` returns that tree plus any native callbacks. `null` as an ordering limit
is the fixture convention for the builder's `Infinity`, because JSON.stringify loses Infinity.
To produce an actual zero-length slice, use a numeric zero limit.

The runtime calls `getAllEntities` and uses the existing state and definition handles. It keeps
JS Set semantics for the operators that evaluate outside the enclosing universe. Missing numeric
variables are NaN. Callback property reads distinguish undefined from numeric NaN internally.
Ordering follows the TimSort comparison schedule in Node 26.11.1 / V8 14.6; Piovium's comparator
can be non-transitive when a key is NaN, making an arbitrary stable sort observably different.
The reference algorithm is [V8's array-sort.tq](https://github.com/nodejs/node/blob/v26.11.1/deps/v8/third_party/v8/builtins/array-sort.tq).

Native C++ callbacks are the intended translation of JS `var` predicates and `orderByFn` functions.
For JSON `fn` specifications, the portable parser accepts single-parameter arrows and functions,
simple return blocks, variable property access, arithmetic, numeric comparisons, boolean primitives,
short-circuit operators, ternaries, `Number`, `Number.isNaN`, `Number.isFinite`, and selected `Math`
operations (`abs`, `floor`, `ceil`, `trunc`, `round`, `min`, `max`). It also understands the core's
named-variable predicate wrapper and its two special symbol keys. It is **not a general JavaScript
runtime**: arbitrary statements, objects, arrays, strings as results, closures and other JS syntax
are unsupported. Supply translated callbacks through the builder or bind a `QueryExpression::functions`
entry to an exact serialized function string. Unsupported syntax throws instead of returning guesses.

Attached-card cost queries use executable `EntityDefinition::modifications` when available. The static
definition dump lacks these functions, so fixture tests skip those cost cases. Other attachment
queries, including `on`, `with`, tags, variables and area filters, are checked normally.

The test loaders own their definitions for the lifetime of loaded states. `loadInternalState` retains
the full raw fixture JSON alongside the `GameStatePointer`. `phaseDamageLog` and `phaseReactionLog`
remain in that JSON graph because the foundation only forward-declares their event classes. Config
fields absent from the foundation's `GameConfig` also remain in the raw JSON. Queries use none of
those fields. The fixtures are query snapshots, not resumable C++ games.

Testing the queries against Node: [docs/verification.md](../docs/verification.md).
