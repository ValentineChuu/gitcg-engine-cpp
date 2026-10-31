// Tallies reactions (and their causes) and damage per player over a game (reference/bridge/combat_log.mjs): the
// per-phase logs are read just before each clear and once at the end, each event counted once.
#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "core/json/json_value.h"
#include "core/state/game_state.h"

namespace gitcg {

class CombatLog {
 public:
  void scan(const GameStatePointer& state);
  // {reactions, reactionCauses, damageDealt} in the finished message's shape.
  JsonValue reactionsJson() const;
  JsonValue reactionCausesJson() const;
  JsonValue damageDealtJson() const;

 private:
  // Owning references: a freed event's address could otherwise be reused by a new one and be skipped.
  std::set<std::shared_ptr<const void>> seen_;
  std::map<int, int> reactions_[2];                               // reaction code -> count (JS orders integer keys)
  std::vector<std::pair<std::string, int>> reactionCauses_[2];  // insertion ordered
  double damageDealt_[2] = {0, 0};
};

}  // namespace gitcg
