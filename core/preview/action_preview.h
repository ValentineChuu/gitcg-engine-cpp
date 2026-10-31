// Cost precalculation and optional action previews (core/src/preview.ts).
#pragma once
#include "core/events/action_info.h"
#include "core/state/game_state.h"
namespace gitcg {
ActionInfo modifyAndPreview(const GameStatePointer& state, const ActionInfo& action, bool enablePreview);
}
