# gitcg-engine-cpp

A fast C++ engine for **Genius Invokation TCG**, built for bots: it runs thousands of games at once in one process,
driven over stdio, with game copies for search and extra inputs for neural networks. It is a port of
[Piovium's engine](https://github.com/piovium/genius-invokation) (`@gi-tcg/core` and `@gi-tcg/data` 0.21.0) plus
NPC-only characters Piovium doesn't have, and it plays exactly the same games, about 22 times faster.

It was written for AutoGITCG, a reinforcement-learning bot for the game, which includes it as a submodule at
`engine/`.

## Build and run

```sh
make -j8 build/release/worker                    # g++ with C++23
make -j8 build/release/worker LDFLAGS=-static    # a static binary for machines without a compiler
make MODE=debug                                  # address and undefined-behaviour sanitizers
```

Nothing else is needed to run it: the card code is already translated to C++ (`cards/`), and the one data file the
worker reads is in `data/`. Start `build/release/worker` and talk to it in JSON lines (below).

## The worker protocol

One JSON message per line on stdin, one per line on stdout. The full message list is at the top of
`reference/bridge/worker.mjs`; in short:

- `create` a game from two decks and a seed (optional: initial equipment, action previews, the opponent's replies
  with a view of the opponent's cards per seat, a seat played by a person);
- the worker sends a `decision` for each player choice: the request (legal options, each action with its cost and a
  legal dice payment), that player's own view of the state, entity counters, and the optional inputs;
- `respond` with the choice; `copy` a game at a pending action decision with the hidden cards re-dealt (for
  look-ahead search); `replies` computes the opponent's replies for chosen actions on demand;
- `finished` (winner, rounds, characters defeated, reactions and damage per player) or `failed` ends a game.

Games are reproducible from their seed and responses. Decks are shuffled by the bridge's seeded generator, and
answers to decisions asked of both players at once (rerolls, opening hands) must be replayed in the same order.

### Inputs computed for bots

- **Previews**: each action's immediate result.
- **Replies**: for an action, the opponent's whole turn after it, as the worst outcome over their possible lines.
  Their hidden cards come from a view: their real list when it is public, or candidate decks sent by the caller (a
  guess), never their real hidden cards.
- **Follow-ups**: the best outcome reachable with the rest of one's own turn.
- **End of round**: what the end phase would do if both players ended the round now.

These come from a search over every distinct line of a player's turn, with rules that cut useless moves (invalid or
effectless moves, healing food on full HP, duplicate copies, one tuning per die; draw and random cards end a line).
The card facts those rules need are `data/card_line_facts.json`.

## Using it

### With AutoGITCG

[AutoGITCG](https://github.com/ValentineChuu/AutoGITCG) includes this repository as a submodule at `engine/`. Build the
worker once (`make -C engine -j8 build/release/worker`); its Python bridge starts the worker processes itself and
speaks the protocol for you. Its README covers the rest.

### On its own

Any language that can start a process and read and write lines can drive the worker. `examples/random_game.py` is the
whole loop in about 80 lines of plain Python: start the worker, `create` a game, answer every `decision` with a random
legal choice, read `finished`.

```sh
make -j8 build/release/worker
python3 examples/random_game.py 7     # one game between two random players; the argument is the seed
```

A real player differs only in how it chooses: each decision carries that player's own view of the board (`state`)
and the request with its legal options, and the answer names one of them. For actions, `autoSelectedDice` is a legal
dice payment the engine already worked out.

### With Piovium's board, for a person to play

Piovium's board UI is published on npm as [`@gi-tcg/web-ui-core`](https://www.npmjs.com/package/@gi-tcg/web-ui-core)
(SolidJS; 0.21.0 matches this engine). Its `createClient(seat)` returns the board component and an `io` object with
`notify` and `rpc`, the same interface Piovium's own engine calls. Create the game with `"humanSeat": <seat>`, and the
worker sends that seat what the board needs:

- each `notify` message's `notification` goes to `io.notify(...)` unchanged: the person's view of the board and the
  mutations that drive its animations (cards played, skills, damage, reactions), the same notifications Piovium's
  engine sends, at the same points;
- each `decision` for that seat goes to `io.rpc({request: decision.request})`; it resolves once the person has chosen,
  with `{response: ...}`, and that inner `response` is what the worker's `respond` message takes.

The worker runs on a server, so a small relay between it and the browser is needed (a WebSocket per game is enough);
it isn't part of this repository. To try Piovium's own engine and board first, see
<https://standalone.piovium.org>.

## How it is built

- **State is a persistent value.** Players, characters, entities, dice and hands are immutable reference-counted
  nodes; a change copies only the path from the root, so copying a game is copying the root (`core/state`,
  `core/mutations`). The same state gives the same exposed-state JSON as the reference, field by field
  (`core/expose`).
- **Determinism, bit for bit.** The same minstd random generator called at the same points in the same order, the
  same entity id allocation and the same event order as the reference. Any divergence shows up in the equivalence
  tests as the first step that differs.
- **Control flow.** Piovium's engine is `async` because a request (choose a card, choose a character) can come in the
  middle of a skill. This engine calls a synchronous interface instead; the worker gives each game its own fiber that
  suspends at a request (`bridge/fibers`). Copies always restart from the state at the start of a step.
- **Card data is translated, not hand-written.** `tools/translator/` reads the GTS card sources through the GTS
  compiler, turns each definition's attribute tree into C++ registration code (`cards/generated/`, the definition VM
  in `definitions/`) and translates the card code's lambdas into C++ with the TypeScript compiler API, type-directed
  (`cards/translated/`, `docs/lambda_translator.md`). A few pieces are ported by hand (`cards/manual/`). Generated
  files are never edited: regenerate them (below).
- **The query language** (`$.opp.character.def(X)` and so on) is its own port in `query/`.

`docs/core_semantics.md` records the engine behaviour the port has to keep, with references into Piovium's source.

| Folder | Contents |
|---|---|
| `core/` | state, mutations, events, skills, game loop, rules (costs, dice, reactions), previews, exposed state, JSON |
| `query/` | the `$` query language |
| `definitions/` | the definition VM: builders and registry for characters, skills, entities and cards |
| `cards/` | generated registration code, translated card code, hand ports |
| `bridge/` | the worker protocol: sessions, copies and re-deals, fibers, turn search, replies, follow-ups |
| `programs/` | the worker, and check and benchmark programs (`support/`: their shared helpers: JSON files, loading saved states and definitions, benchmark processes) |
| `data/` | the card facts the line rules read |
| `tools/translator/` | the GTS and TypeScript to C++ translator and its coverage tools |
| `tests/` | recording games on the reference and replaying them here; definition comparison; benchmarks |
| `reference/` | the reference implementation (below) |

## The reference (`reference/`)

You only need this to update the cards or to rerun the equivalence tests. It is the JavaScript version this engine
was ported from and is checked against: Piovium's engine with a small patch set (`reference/patches/`), the
bot-facing worker written for it (`reference/bridge/`, which the C++ `bridge/` ports), and the GTS definitions of
the extra NPC characters (`reference/custom_data/`; `variants/` is generated). It is also the input of the
translator.

```sh
npm install            # at the engine root: Piovium's packages; then reference/patches/apply_core_patches.mjs patches the core
npm run build:data     # clones Piovium at v0.21.0 into reference/piovium and rebuilds its card data with custom_data/
node reference/tools/export_card_facts.mjs     # refreshes data/card_line_facts.json
node reference/tools/try_characters.mjs --games 20 <character ids>   # random games with a lineup: failures and skill effects
```

After the reference's cards change, regenerate the C++ cards with `node tools/translator/translate.mjs` and
`node tools/translator/emit_bindings.mjs`, rebuild, and rerun the checks.

Some tools, tests and benchmarks read game data that isn't part of this repository (Dimbreath's game tables, test
matchups): they look in `GITCG_DATA` / `GITCG_RUNS`, else in the `data/` and `runs/` folders of the repository the
engine is checked out in (`reference/data_paths.mjs`). Standalone, point `GITCG_DATA` at a folder holding
`raw/GCGDeckExcelConfigData.json`, which AutoGITCG's `scripts/data/fetch_dimbreath.py` downloads.

## Tests

```sh
npm install                                 # once, at the engine root (the tests use the reference's packages)
node tests/replay_reference.mjs tests/fixtures/starter_protocol.jsonl.gz   # this engine against recorded reference games
node tests/record_reference.mjs --games 20 --human-seat 0 --output /tmp/notify.jsonl.gz   # reference games with a person's seat
node tests/replay_reference.mjs /tmp/notify.jsonl.gz                       # ...their notifications must match too
cd reference
node tests/patch_equivalence.mjs 30        # patched and unpatched reference cores play identical games
node tests/replies_equivalence.mjs 8       # previews and replies don't change the game
node tests/engine_cache_equivalence.mjs    # the cache patch on and off: identical games and turn searches
```

What is checked, the full suites and the speed measurements: [docs/verification.md](docs/verification.md).

## License

AGPL-3.0 (see `LICENSE`), as Piovium's engine is. Genius Invokation TCG is HoYoverse's; this is an unofficial fan
project.
