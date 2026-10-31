#include "core/state/player_state.h"

namespace gitcg {

namespace {

template <typename Player>
auto& flagField(Player& player, PlayerFlag name) {
  switch (name) {
    case PlayerFlag::DeclaredEnd: return player.declaredEnd;
    case PlayerFlag::HasDefeated: return player.hasDefeated;
    case PlayerFlag::CanCharged: return player.canCharged;
    case PlayerFlag::CanPlunging: return player.canPlunging;
    case PlayerFlag::LegendUsed: return player.legendUsed;
    case PlayerFlag::SkipNextTurn: return player.skipNextTurn;
    case PlayerFlag::DefeatedSwitching: return player.defeatedSwitching;
  }
  return player.declaredEnd;
}

}  // namespace

bool PlayerState::flag(PlayerFlag name) const {
  return flagField(*this, name);
}

void PlayerState::setFlag(PlayerFlag name, bool value) {
  flagField(*this, name) = value;
}

}  // namespace gitcg
