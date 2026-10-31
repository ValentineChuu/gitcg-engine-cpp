#include "core/rules/dice_order.h"

#include <algorithm>
#include <map>

#include "core/rules/character_traits.h"

namespace gitcg {

namespace {

std::map<DiceType, int> countByType(const std::vector<DiceType>& dice) {
  std::map<DiceType, int> counts;
  for (DiceType type : dice) ++counts[type];
  return counts;
}

}  // namespace

std::vector<DiceType> sortDice(const PlayerState& player, const std::vector<DiceType>& dice) {
  std::set<DiceType> useful = playerUsefulDice(player);
  std::map<DiceType, int> counts = countByType(dice);
  auto key = [&](DiceType type) {
    return std::array<int, 4>{type == dice::Omni ? -1 : 0, useful.contains(type) ? -1 : 0, -counts[type], diceIndex(type)};
  };
  std::vector<DiceType> sorted = dice;
  std::stable_sort(sorted.begin(), sorted.end(), [&](DiceType first, DiceType second) { return key(first) < key(second); });
  return sorted;
}

std::vector<DiceType> computeConvertDice(const PlayerState& player, DiceType target, int count) {
  std::vector<DiceType> protectedDice;
  std::vector<DiceType> remaining;
  for (DiceType type : player.dice) {
    if (type == dice::Omni || type == target) protectedDice.push_back(type);
  }
  for (DiceType type : player.dice) {
    if (std::find(protectedDice.begin(), protectedDice.end(), type) == protectedDice.end()) remaining.push_back(type);
  }
  std::set<DiceType> useful = playerUsefulDice(player);
  std::map<DiceType, int> counts = countByType(remaining);
  auto key = [&](DiceType type) { return std::array<int, 3>{useful.contains(type) ? 1 : 0, counts[type], diceIndex(type)}; };
  std::stable_sort(remaining.begin(), remaining.end(), [&](DiceType first, DiceType second) { return key(first) < key(second); });
  int converted = count < 0 ? static_cast<int>(remaining.size()) : std::min<int>(count, static_cast<int>(remaining.size()));
  std::vector<DiceType> result = protectedDice;
  result.insert(result.end(), remaining.begin() + converted, remaining.end());
  result.insert(result.end(), converted, target);
  return sortDice(player, result);
}

}  // namespace gitcg
