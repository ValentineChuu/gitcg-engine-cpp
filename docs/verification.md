# How the engine is checked against the reference

The engine must play exactly the games the reference (`reference/`, Piovium's JavaScript engine) plays. Every check
replays recorded reference games through the C++ worker and compares every message: each decision's request and state, previews, replies, copies, results.
Commands run from the engine repository's root after `make -j8`.

## Quick check (fixtures in the repository)

```sh
node tests/replay_reference.mjs tests/fixtures/starter_protocol.jsonl.gz      # 14 games
node tests/replay_reference.mjs tests/fixtures/focused_protocol.jsonl.gz      # 7 games: copies, replies
node tests/replay_reference.mjs tests/fixtures/wide_differences.jsonl.gz      # 6 games that once differed
node tests/replay_reference.mjs tests/fixtures/sweep_difference.jsonl.gz --snapshots
build/release/copy_state_check 100     # game copies restart from unchanged states
build/release/forced_copy_check        # a copy whose first decision doesn't match fails once and stops
```

Each replay prints a summary such as `{"games":14,"identical":14,"untranslated":0,"different":0}`.

## Full suites (recorded locally)

The large recordings are generated, not stored. They need the reference set up (`reference/`) and the game data
(`GITCG_DATA`, see `reference/data_paths.mjs`); the deck exports use AutoGITCG's Python package
(`tests/export_test_decks.py`, `tests/export_stage_coverage.py`).

| Recording | Made by | Games | Decisions |
|---|---|---|---|
| reference_1, reference_2 | `tests/record_reference.mjs` (tournament, NPC, stage and synergy decks) | 40 + 200 | 40,530 |
| wide | `tests/record_protocol.mjs --games 2400` (copies, previews, every reply mode) | 2,400 | 404,531 |
| sweep, sweep with opening hands | `tests/record_sweep.mjs` (every card played in a short game) | 1,546 + 16 | 40,402 |
| every NPC stage | `tests/record_stage_coverage.mjs` | 1,162 | |
| slot probes | `tests/record_slot_probes.mjs` (positions that reach rarely used card code) | | |

`python3 tests/verify_finish.py --suites all` replays all of them (and the fixtures) and fails on any difference
or any card code without a C++ translation; `python3 tests/verify_coverage.py` does the stage and probe
recordings. `node tests/compare_definitions.mjs` compares every card definition the two engines build (268
characters, 1,272 entities, 4,300 skills).

## Results

- Every suite above is identical: 0 differences, 0 untranslated stops.
- 3,404 of the 3,406 pieces of card code (lambdas) in the current version actually run during the recordings, and
  match. The other two are a defeat handler that Piovium's own default filters block, in Node as well.
- 174 code slots are not bound to a C++ function; each is classified in `docs/coverage_inventory.json`: code from
  older card versions, code compiled into a parent that is bound, or helpers replaced at every caller. None is
  reachable in the current version.
- One protocol field differs on purpose: a card move's mutation `reason` code in previews (Node 4, C++ 0). Nothing
  reads it.

## Speed

On the same 720 random NPC games, 12 processes: about 22 times as many games per second as the patched reference.
In training (network decisions per second, with previews and replies) 35-45 times. `tests/benchmark_twelve.py`
and `tests/benchmark_suite.py` measure it; results from the porting work are in `docs/protocol_results/`.
