// How a game asks its players (core/src/game.ts rpc): one call per request type. An implementation may suspend
// the calling fiber until the answer arrives; invalid answers are rejected by the game (the player loses).
#pragma once

#include <vector>

#include "core/events/action_info.h"
#include "core/values/game_enums.h"

namespace gitcg {

struct ActionAnswer {
  int chosenActionIndex = 0;
  std::vector<DiceType> usedDice;
};

class PlayerIo {
 public:
  virtual ~PlayerIo() = default;
  virtual ActionAnswer action(int who, const std::vector<ActionInfo>& actions) = 0;
  virtual int chooseActive(int who, const std::vector<int>& candidateIds) = 0;
  virtual std::vector<DiceType> rerollDice(int who) = 0;
  virtual std::vector<int> switchHands(int who) = 0;
  virtual int selectCard(int who, const std::vector<int>& candidateDefinitionIds) = 0;
};

}  // namespace gitcg
