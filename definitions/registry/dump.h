// Definition metadata equivalent to tests/dump_definitions.mjs; functions and target getters stay opaque.
#pragma once
#include "core/definitions/game_data.h"
#include "core/json/json_value.h"
namespace gitcg { JsonValue dumpDefinitions(const GameData& data); }
