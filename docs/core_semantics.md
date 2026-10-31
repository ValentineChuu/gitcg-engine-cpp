# Core semantics the port must keep (notes from reading @gi-tcg/core 0.21.0)

Line references are to `reference/piovium/packages/core/src`. Our patches are in `reference/patches/apply_core_patches.mjs`.

## State and mutations

- State is an immutable tree (immer). Code keeps old snapshots on purpose: every event arg holds `onTimeState`,
  `removedEntities` keeps an entity's last state, mutations write refreshed references back (`m.state`,
  `m.oldValue`, `value.id` of a created entity). The C++ state is therefore a persistent copy-on-write tree:
  immutable reference-counted nodes, a change copies only the path from the root. Copying a game = copying the root.
- Ids: initial ids count down from -500000 (characters of player 0, then its pile, then player 1's), then
  `iterators.id` continues from there (`stepId` returns the current value and decrements; created entities with
  id 0 take `iterators.id--`).
- Random: minstd, `x -> 48271 x mod (2^31 - 1)`, `stepRandom` returns the new value. `randomDice(n)` =
  `stepRandom() % 8 + 1` each (unless alwaysOmni). Pile insert "random": `stepRandom() % (pile.length + 1)`.
- `modifyEntityVar` outside int32 is silently skipped (mutation.ts doMutation).
- `resetDice` stores `sortDice(player, value)`; dice order depends on the player's characters (utils).
- `variables` are JS objects: key order = insertion order (definition `varConfigs` order first, later keys
  appended). Any code that iterates variables (exposeState, recreate) must keep that order.
- Extensions hold arbitrary JSON-like state (`pair<number[]>` etc.); mutated by `mutateWhen` hooks.

## Game loop (game.ts)

- `start()`: notifyAndPause, then per step: [patch 6: `onStepStart(state)`], run the phase function, then
  `changePhase`, `clearRemovedEntities`, [patch 5: `beforeClearPhaseLogs(state)`], `clearPhaseLogs`, notifyAndPause.
- initHands: draw 5 for p0 then p1 (handleEvents each), then both `switchHands` concurrently (`Promise.all`).
- initActives: both `chooseActive` concurrently; [patch 3: both `mutator.switchActive` first, then handle s0 then
  s1 events]; `onBattleBegin`; `stepRound`.
- roll: `onRoundBegin`; `modifyRoll` per player (p0 then p1); then per player concurrently: roll dice (random
  calls happen synchronously before the first await, so p0's 8 dice then p1's 8 dice), resetDice, reroll loop.
  Reroll random calls happen when the answer arrives, so the order depends on which player answers first.
  The C++ worker runs concurrent branches as fibers resumed in message-arrival order.
- action: `onActionPhase` if prevPhase was roll; declaredEnd -> switchTurn; skipNextTurn -> clear + switchTurn;
  else `onBeforeAction`, set canCharged = dice even, [patch 5 hook], `clearPhaseLogs`, replace-action check,
  else availableActions -> rpc -> modifyAction0..4 on the chosen action's eventArg -> dice check -> consume dice,
  energy, legend -> per type (useSkill: onBeforeUseSkill, executeSkill, onUseSkill; playCard: onBeforePlayCard,
  effectless removal or executeSkill, onPlayCard; switchActive; elementalTuning; declareEnd) -> onAction ->
  switchTurn unless fast. Both declaredEnd -> clear both flags, go to end.
- end: onEndPhase; draw 2 for currentTurn then the other; onRoundEnd; clear hasDefeated/canPlunging; clearRoundLogs;
  stepRound; roundNumber >= 15 -> gameEnd.
- availableActions order: initiative skills (per target), then per hand card: play (per target) then tuning,
  then switches (alive, not active, character order), then declareEnd. Then each goes through
  `ActionPreviewer.modifyAndPreview` [patch 1: previews only with enablePreview; modifyAction0-4 always] and
  `applyAutoSelectedDiceToAction`.

## Mutator (mutator.ts)

- `EventList.push` merges damage events with the same target into one (summed value, or-ed causeDefeated and
  fromReaction, keeps the first event's onTimeState).
- Inline events (modifyDamage0..3, modifyHeal0/1, modifyReaction, ...) run `allSkills(state, event)`, filter, and
  execute each skill on its own context; the result state replaces the mutator's state (`resetState`).
- damage: modifiers modifyDamage0, then the reaction ($REACTION: `increaseDamageByReaction`, records the reaction
  log), then modifyDamage1..3; health = max(0, health - value); push damage log; then `apply` the element (not for
  physical/piercing).
- heal: revive sets alive first; [patch 4: health 0 and not immuneDefeated -> skip]; final = min(value, injury);
  `stepId` for the heal id; modifyHeal0/1; cancelled -> stop.
- insertEntityOnStage: immuneControl blocks disableSkill statuses; existing entity of the same definition ->
  rewrite variables; summons/supports capped at 4; onEnter only when not moved.
- switchHands: each returned card goes to `stepRandom() % (pile.length + 1)`, then draw `count` cards from the top
  skipping definitions just swapped in.

## Skill execution (skill_executor.ts, runtime/skill_context.ts)

- A skill action runs on its own `SkillContext` with its own mutator over a copy of the state; it returns
  `[newState, {emittedEvents, innerNotify, error, causeDefeated, mainDamage}]`, and the executor adopts the state.
- executeSkill: character initiative skill (normal/elemental/burst, not technique) first sets canPlunging = false.
- finalizeSkill (character initiative only): pushRoundSkillLog with the caller as it was at the start (intentional
  bug: the old definition id), then +1 energy if gainEnergy and alive and no specialEnergy (capped at maxEnergy);
  then handle the emitted events; then, if this skill caused a defeat, both players whose active character is dead
  set defeatedSwitching and choose a new active concurrently (`Promise.all`), switch in order currentTurn, other
  (handling each switch's events), then restore the saved defeatedSwitching flags.
- handleEvent: requests (requestReroll, requestSwitchHands, requestSelectCard (+ onSelectCard), requestUseSkill
  (onBeforeUseSkill, finalize, onUseSkill; skipped if disabled or not available), requestPlayCard,
  requestAdventure, requestTriggerEndPhaseSkill); any other event: for a DisposeEventArg the disposed entity's own
  skills for that event first, then `allSkills(state, name)` collected ONCE at the start; for each: set
  `arg._currentSkillInfo`, check the filter on the CURRENT state, finalize. Stop when phase is gameEnd.
- Event preprocessing at skill end (finalizeMode "standard"): damage events that would defeat run the inline
  `modifyZeroHealth` (immune possible); defeated -> alive 0, energy (or specialEnergy variable) 0, aura none,
  hasDefeated flag, removeRoundSkillLog; players with no alive characters lose (both -> gameEnd, no winner).
  Emitted order: other events, then hand-card-inserted events (dropped if the card left that player's hand,
  unless overflowed), then safe damage events, then critical (defeating) damage events.
- `allSkills` order: extensions' skills first (caller = current turn player's active character, only if one is
  set), then entities by `getAllEntities` (or characters only for onBattleBegin). getAllEntities order: for
  currentTurn then the other player: dead characters (active-first rotation) and their entities; then per player
  in that order: active character + entities, combat statuses, standby characters (rotation order) + entities,
  summons, supports, hands (+attachments), pile from bottom to top (+attachments).
- getEntityById searches per player: characters (each then its entities), combatStatuses, summons, supports,
  hands, pile, removedEntities (with attachments of each). Throws when missing.

## Reactive views (runtime/reactive)

- Card code never sees raw states: `applyReactive` turns character/entity/attachment states into (context, id)
  objects that read the LATEST state by id on every access; plain objects/arrays reached through them are proxied
  the same way (fields starting with `_` are not). `onTimeState` and `EnterEventArg.overridden` are NoReactive.
  C++: id handles over the context's current state.
- Area lookups are cached per context but invalidated on move/remove: semantically the same as recomputing.

## Values that must match JS exactly

- DiceRequirement is a JS Map: insertion order matters (normalizeCost deletes zero entries, and an empty map
  becomes {Aligned: 0}; appendCost adds to the first existing type in priority order, else Aligned).
- Damage: value = ceil((value + increased) * ((multiplied ?? 1) / divider)) in doubles, then max(0, v - decreased);
  multiplyDamage adds to `multiplied` starting from 0 (two x2 = x4). causeDefeated = value >= target health.
- Heal modifier: expected = max(0, ceil(expected + increased - decreased)), value = min(expected, max - health).
- Skill ids can be fractional (triggered skills); exposed as Math.floor.
- `sortDice`: omni first, then useful (alive characters' elements), then higher count, then dice index.

## Definitions from GTS (gts/vm_impl, data/registry)

- Translator input: `@gi-tcg/gts-transpiler` `transpile(source, file, {runtimeImportSource, providerImportSource})`
  gives readable TS: per definition an attribute tree `{name, positionals: () => [...], named: {attributes}}`
  whose leaves include lambdas `(__gts_fnArg) => {...}`. Type-check it with the TS compiler API to translate
  lambdas into typed C++. The C++ VM interprets the same attribute calls in the same order.
- Triggered skill ids are sub-ids `mainId + 0.01 * n` from a GLOBAL per-mainId counter (vm_impl/sub_id.ts), so
  registration order (data/src/index.ts import order, old_versions included) decides them. Verify with a static
  dump of every definition from both sides.
- EntityModel.getSkills (at getEntry, after all `on`s): if disposeOnMasterDefeated (status/equipment unless
  `noDefaultDispose`), a "defeated" dispose skill (bypassDefeatedFilter) gets the next sub-id and is put FIRST;
  then if it has usagePerRound* or duration variables a "roundEnd" skill (reset per-round usages to initial, then
  duration -1 and dispose at <= 0) gets the next sub-id and is put LAST.
- Attribute side effects with their own sub-ids, in attribute order: prepare (replaceActionBySkill + switchActive),
  nightsoulsBlessing autoDispose (beforeAction, listenTo all), shield (decreaseDamaged), elementalBlessing
  (actionPhase, hand/pile triggering), hint dynamicPreset (dealDamage/damaged), conflictWith (selfEnter), on/once.
- usage: name "usage" (perRound: usagePerRound, usagePerRound1, ... by index); autoDispose -> disposeWhenUsageIsZero.
  Variables: visibleVarName = last variable set without `visible: false`. Variable config: range n -> [0, n],
  [a, b]; append true/number/{limit, value} -> recreate "append"; forceOverwrite -> "overwrite"; else "default".
- Registry.resolve: per id, the first version resolver that accepts one of the registered versions wins;
  characters get their skills by skillIds: initiative skills as-is, passive skills' skills appended and their
  varConfigs merged into the character's.
- Detailed events (runtime/skill.ts detailedEventDictionary) = core event + filter on the listen relation
  (myself / sameArea / samePlayer / all, checkRelative) evaluated on `e.onTimeState`.
