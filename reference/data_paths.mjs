// Where the game data the engine's tools and tests read or write lives: the folder in GITCG_DATA (holding raw/ and
// derived/), else the data/ folder of the repository the engine is checked out in (AutoGITCG keeps it at engine/).
// Test matchups and other run outputs: GITCG_RUNS, else that repository's runs/ folder.

import { join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const ENGINE_ROOT = fileURLToPath(new URL("..", import.meta.url));

export const DATA_DIRECTORY = resolve(process.env.GITCG_DATA ?? join(ENGINE_ROOT, "..", "data"));
export const RUNS_DIRECTORY = resolve(process.env.GITCG_RUNS ?? join(ENGINE_ROOT, "..", "runs"));

export const dataFile = (...parts) => join(DATA_DIRECTORY, ...parts);
export const runsFile = (...parts) => join(RUNS_DIRECTORY, ...parts);

// Dimbreath's NPC deck table (fetched by AutoGITCG's scripts/data/fetch_dimbreath.py): decks for tests and benchmarks.
export const NPC_DECK_TABLE = dataFile("raw", "GCGDeckExcelConfigData.json");
