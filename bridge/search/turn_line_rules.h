// The bridge's exported card facts and deterministic branch trimming.
#pragma once
#include "core/events/action_info.h"
#include "core/state/game_state.h"
namespace gitcg {
bool endsLine(const ActionInfo& action);
bool cardIsLeftOutOfReplies(const ActionInfo& action);
std::vector<int> branchingMoves(const std::vector<ActionInfo>& actions, const GameStatePointer& state);
std::string cardOutcomeKey(const ActionInfo& action);
}
