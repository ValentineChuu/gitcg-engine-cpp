// The whole game (core/src/base/state.ts GameState). Immutable once built: a change makes a new root that shares
// every untouched node, so a snapshot or a copy of the game is one pointer.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "core/json/json_value.h"
#include "core/state/player_state.h"
#include "core/state/ref_pointer.h"
#include "core/values/game_enums.h"

namespace gitcg {

struct GameData;
struct ExtensionDefinition;

struct GameConfig {
  int64_t randomSeed = 0;
  int initialHandsCount = 5;
  int maxHandsCount = 10;
  int maxPileCount = 200;
  int maxRoundsCount = 15;
  int maxSupportsCount = 4;
  int maxSummonsCount = 4;
  int initialDiceCount = 8;
  int maxDiceCount = 16;
  // unexpectedInsufficientDice is "skipConsume" and hostRelatedExecution is off in every game we play.
};

// Rules that changed between game versions (core/src/base/state.ts VersionBehavior); all games use v7.1.0.
struct VersionBehavior {
  bool recreateTakesMax = true;  // defaultRecreateBehavior "takeMax" (from v3.5.0), else "overwrite"
  bool foodOmitInjuredOnly = true;
  bool discardMaxCostHandsAbortPreview = true;
  bool diceCostApplyAttachments = true;
};

struct ExtensionState {
  const ExtensionDefinition* definition = nullptr;
  JsonValue state;
};

struct GameState : RefCounted {
  const GameData* data = nullptr;
  GameConfig config;
  VersionBehavior versionBehavior;
  int64_t randomIterator = 0;  // iterators.random
  int idIterator = 0;           // iterators.id, counts down
  Phase phase = Phase::InitHands;
  std::optional<Phase> previousPhase;
  int roundNumber = 0;
  int currentTurn = 0;
  std::optional<int> winner;
  std::array<PlayerPointer, 2> players;
  std::vector<ExtensionState> extensions;
};

using GameStatePointer = RefPointer<GameState>;

}  // namespace gitcg
