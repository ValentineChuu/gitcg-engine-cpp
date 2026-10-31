// Patches the built @gi-tcg/core bundle for training speed. Runs automatically after `npm install`.
//
// 1. Action previews are skipped unless `new Game(state, { enablePreview: true })`.
//    Cost modifiers (modifyAction0-4) still run, so validity and dice costs stay correct.
// 2. A player whose `io.notify` is set to `null` receives no notifications,
//    which skips building that player's exposed state on every mutation.
// 3. Fix: at game start both active characters are set before either player's switch events run.
//    Before, seat 0's events ran while seat 1 had no active character, so a pre-equipped talent reacting
//    to the switch (Serious Showdown Lisa, 214091, targets the opponent's active) crashed the game.
// 4. Fix: a normal heal on a character at 0 HP who isn't marked defeated yet (it fell earlier in the same
//    attack) is skipped, as for defeated characters, instead of throwing. Seen with Splendor of Tranquil
//    Waters (311508) healing its fallen wielder. immuneDefeated heals still require 0 HP.
// 5. Hook: `game.beforeClearPhaseLogs(state)`, if set, runs just before the per-phase damage/reaction logs
//    are cleared, so the bridge can record every damage and reaction event (bridge/combat_log.mjs).
// 6. Hook: `game.onStepStart(state)`, if set, runs at the top of each step of the game loop (one phase step;
//    in the action phase, one turn with exactly one action decision). A `new Game(state)` started from that
//    state replays the step, so look-ahead search can copy a game at an action decision (bridge/game_session.mjs).
// 7. Speed: four board scans the engine repeats for every event are cached per game state (about 30%
//    of engine time in a turn search): all entities (On), the skills an event triggers (jn), an entity's area (V)
//    and an entity by id (B). Finished states are frozen by immer, so the same state always gives the same answer;
//    drafts (marked with Symbol.for("immer-state")) bypass the cache, lists are handed out as copies, and a missing
//    id falls back to the original scan (which throws as before). `globalThis.autogitcgEngineCache = false` turns the
//    cache off (tests/engine_cache_equivalence.mjs compares games with it on and off).
//
// Each patch is applied once; patches already present are skipped, so new ones apply to patched installs.
// Anchors are exact strings from the pinned core version; any mismatch aborts loudly.

import { readFileSync, readdirSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const PINNED_CORE_VERSION = "0.21.0";
const PATCH_MARKER = "/* autogitcg-patched */";

const PATCHES = [
  {
    name: "pass enablePreview option into the action previewer",
    find: "c=new _i(this.state,e,s);return await Promise.all(r.map(n=>c.modifyAndPreview(n)",
    replace: "c=new _i(this.state,e,s,this.option.enablePreview===true);return await Promise.all(r.map(n=>c.modifyAndPreview(n)",
  },
  {
    name: "store enablePreview on the action previewer",
    find: "_i=class{constructor(e,t,n){this.originalState=e,this.who=t,this.skipError=n}",
    replace: "_i=class{constructor(e,t,n,p){this.originalState=e,this.who=t,this.skipError=n,this.enablePreview=p}",
  },
  {
    name: "return after cost modifiers when previews are disabled",
    find: "await r.precalculateEvent(`modifyAction4`,t);let i=t.action,",
    replace: "await r.precalculateEvent(`modifyAction4`,t);if(!this.enablePreview)return{...t.action,eventArg:n};let i=t.action,",
  },
  {
    name: "skip notifications for players with io.notify === null",
    find: "notifyOneImpl(e,t){let n=this.players[e].io,r=",
    replace: "notifyOneImpl(e,t){let n=this.players[e].io;if(n.notify===null)return;let r=",
  },
  {
    name: "set both initial active characters before handling their switch events",
    find: "return this.mutator.postChooseActive(t,n),await this.switchActive(0,t,null),await this.switchActive(1,n,null),",
    replace: "this.mutator.postChooseActive(t,n);let s0=this.mutator.switchActive(0,t,{fast:null}),s1=this.mutator.switchActive(1,n,{fast:null});return await this.handleEvents(s0),await this.handleEvents(s1),",
  },
  {
    name: "skip normal heals on characters at 0 health awaiting defeat",
    find: "else return{events:i,causeDefeated:a};else if(t.variables.health===0!=(n.kind===`immuneDefeated`))throw",
    replace: "else return{events:i,causeDefeated:a};else if(t.variables.health===0&&n.kind!==`immuneDefeated`)return{events:i,causeDefeated:a};else if(t.variables.health===0!=(n.kind===`immuneDefeated`))throw",
  },
  {
    name: "call beforeClearPhaseLogs at phase changes",
    find: "this.mutate({type:`clearRemovedEntities`}),this.mutate({type:`clearPhaseLogs`})",
    replace: "this.mutate({type:`clearRemovedEntities`}),this.beforeClearPhaseLogs?.(this.state),this.mutate({type:`clearPhaseLogs`})",
  },
  {
    name: "call beforeClearPhaseLogs before each action",
    find: "value:n().dice.length%2==0}),this.mutate({type:`clearPhaseLogs`})",
    replace: "value:n().dice.length%2==0}),this.beforeClearPhaseLogs?.(this.state),this.mutate({type:`clearPhaseLogs`})",
  },
  {
    name: "call onStepStart at the top of each game loop step",
    find: "for(await this.mutator.notifyAndPause({force:!0,canResume:!0});!this._terminated;){let t=",
    replace: "for(await this.mutator.notifyAndPause({force:!0,canResume:!0});!this._terminated;){this.onStepStart?.(this.state);let t=",
  },
  {
    name: "cache all entities per finished state",
    find: "function On(e,t,n){let i=[],a=[],o;",
    replace: "const autogitcgEntities=new WeakMap;function On(e,t,n){if(globalThis.autogitcgEngineCache===false||e[Symbol.for(`immer-state`)])return autogitcgOn(e,t,n);let c=autogitcgEntities.get(e);if(!c){c=new Map;autogitcgEntities.set(e,c)}let k=(t?2:0)+(n?1:0),r=c.get(k);if(!r){r=autogitcgOn(e,t,n);c.set(k,r)}return r.slice()}function autogitcgOn(e,t,n){let i=[],a=[],o;",
  },
  {
    name: "cache the skills an event triggers per finished state",
    find: "function jn(e,t){let n=[];if(e.players[e.currentTurn].activeCharacterId)",
    replace: "const autogitcgTriggered=new WeakMap;function jn(e,t){if(globalThis.autogitcgEngineCache===false||e[Symbol.for(`immer-state`)])return autogitcgJn(e,t);let c=autogitcgTriggered.get(e);if(!c){c=new Map;autogitcgTriggered.set(e,c)}let r=c.get(t);if(!r){r=autogitcgJn(e,t);c.set(t,r)}return r.slice()}function autogitcgJn(e,t){let n=[];if(e.players[e.currentTurn].activeCharacterId)",
  },
  {
    name: "cache entity areas per finished state",
    find: "function V(e,t){for(let n of[0,1]){let r=e.players[n];for(let e of r.characters)if(e.id===t||",
    replace: "const autogitcgAreas=new WeakMap;function V(e,t){if(globalThis.autogitcgEngineCache===false||e[Symbol.for(`immer-state`)])return autogitcgV(e,t);let c=autogitcgAreas.get(e);if(!c){c=new Map;autogitcgAreas.set(e,c)}let r=c.get(t);if(!r){r=autogitcgV(e,t);c.set(t,r)}return{...r}}function autogitcgV(e,t){for(let n of[0,1]){let r=e.players[n];for(let e of r.characters)if(e.id===t||",
  },
  {
    name: "cache entities by id per finished state",
    find: "function B(e,t){for(let n of e.players){for(let e of n.characters){if(e.id===t)return e;",
    replace: "const autogitcgById=new WeakMap;function B(e,t){if(globalThis.autogitcgEngineCache===false||e[Symbol.for(`immer-state`)])return autogitcgB(e,t);let c=autogitcgById.get(e);if(!c){c=new Map;autogitcgById.set(e,c)}let r=c.get(t);if(r===void 0){r=autogitcgB(e,t);c.set(t,r)}return r}function autogitcgB(e,t){for(let n of e.players){for(let e of n.characters){if(e.id===t)return e;",
  },
];

const engineDirectory = join(dirname(fileURLToPath(import.meta.url)), "..");
const coreDirectory = join(engineDirectory, "..", "node_modules", "@gi-tcg", "core");  // the npm project is the engine root
const coreVersion = JSON.parse(readFileSync(join(coreDirectory, "package.json"), "utf8")).version;
if (coreVersion !== PINNED_CORE_VERSION) {
  throw new Error(`@gi-tcg/core is ${coreVersion}, patches were written for ${PINNED_CORE_VERSION}`);
}

const distDirectory = join(coreDirectory, "dist");
const bundleName = readdirSync(distDirectory).find((name) => /^src-.*\.js$/.test(name));
const bundlePath = join(distDirectory, bundleName);
let source = readFileSync(bundlePath, "utf8");

let applied = 0;
for (const patch of PATCHES) {
  if (source.includes(patch.replace)) continue;
  const occurrences = source.split(patch.find).length - 1;
  if (occurrences !== 1) {
    throw new Error(`patch "${patch.name}": expected 1 anchor match, found ${occurrences}`);
  }
  source = source.replace(patch.find, patch.replace);
  applied += 1;
}
if (applied > 0) {
  writeFileSync(bundlePath, source.startsWith(PATCH_MARKER) ? source : PATCH_MARKER + source);
}
console.log(`@gi-tcg/core ${coreVersion}: ${applied} patches applied, ${PATCHES.length - applied} already present`);
