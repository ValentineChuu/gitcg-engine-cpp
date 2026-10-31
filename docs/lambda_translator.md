# Lambda translator design (card code TS -> C++)

Input: the lambda slots from the attribute layer (`cards/generated/lambda_slots.json`: slot id, GTS file, source
offset, text). For each slot, the typed arrow function in the Volar program (`tools/translator/typed_program.mjs`
`lambdasBySourceOffset`) gives the AST with types from the TypeScript checker. Output: one C++ function per slot,
bound to the slot by id, in `cards/translated/<same path>.cpp`.

## What card code uses (survey 2026-10-27, `tools/translator/survey_lambda_api.mjs`)

- 531 distinct (receiver type, member) pairs. Top: `c.e` 2,018, `damage` 1,046, `self` 887, `$.my` 479,
  `characterStatus` 459, `getVariable` 338, `query` 301, `increaseDamage` 296, `Entity.master` 288,
  `combatStatus` 286, enum members (`DamageType.*`, `DiceType.*`), `$.opp`, `.active`, `dispose`,
  `addVariable`, `$.character`, `useSkill`, `heal`, `drawCards`, `summon`, ...
- Syntax: identifiers, property access, calls, expression statements, blocks, literals, binary expressions
  (`===` 554, `&&` 413, `>=`, `>`, `!==`, `||` 71, `??` 18, `+ - / %`, `**` once), `if` 585, `let`/`const` 690,
  object literals 223 (option bags), element access 182, `as` 120, nested arrows 117, arrays 102, `?.` 97,
  `return` 69, `switch` 15, `for...of` 46, `for` 12, ternaries 26, `new` 23 (Set, Map), destructuring 27,
  spread 12, `while` 1.

## Translation rules

- The context parameter `__gts_fnArg` becomes `SkillContext& c`. `c.e` becomes `c.e<EventArgClass>()` with the
  class from the checker type of `eventArg` (DamageOrHealEventArg, ModifyDamage*EventArg -> ModifyDamageEventArg,
  ...). Event fields that are states come back as live handles (`c.get(state)`), as the JS reactive proxies do.
- Numbers are `double`, booleans `bool`; JS truthiness is explicit (`js::truthy(x)` for numbers/handles/optionals).
  `===`/`!==` on numbers -> `==`/`!=`. Variables: `x.variables.name` and `getVariable("name")` ->
  `getVariable(names::name)` with per-file `static const Name` constants.
- Queries (`$...`) -> the C++ `$` builder from the query port, evaluated with `c.queryAll(...)`/`c.query(...)`
  where the JS evaluates them (context methods take resolved handles in C++).
- Option objects (`{ overrideVariables: { usage: 2 } }`) -> the matching C++ structs (`CreateEntityOptions`, ...).
- Arrays: `js::Array` helpers mirroring `filter/map/some/every/find/includes/at/length/toSorted/reduce` with JS
  semantics (stable sort, `at(-1)`).
- Anything outside the rules is reported per slot (file, line, construct) and that slot is ported by hand in
  `cards/manual/`.

## Order of work

1. Translate expressions/statements for the 20 most used receiver types; report coverage per slot.
2. Grow the rule table until most slots translate; hand-port the rest.
3. Every translated card is checked by the game-equivalence tests (Node reference games, per-card sweep).
