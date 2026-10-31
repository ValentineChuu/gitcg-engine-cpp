#include "core/rules/dice_choice.h"

#include <algorithm>
#include <array>
#include <cstdlib>

#include "core/rules/dice_order.h"

namespace gitcg {

namespace {

struct DiceGroup {
  DiceType type;
  int count;
};

// Groups in first-appearance order (a JS Map built while scanning the dice), without omni and disallowed types.
std::vector<DiceGroup> groupDice(const std::vector<DiceType>& dice, const std::set<DiceType>& disallowedDice) {
  std::vector<DiceGroup> groups;
  for (DiceType type : dice) {
    auto group = std::find_if(groups.begin(), groups.end(), [&](const DiceGroup& existing) { return existing.type == type; });
    if (group == groups.end()) {
      groups.push_back({type, 1});
    } else {
      ++group->count;
    }
  }
  std::erase_if(groups, [&](const DiceGroup& group) { return disallowedDice.contains(group.type) || group.type == dice::Omni; });
  return groups;
}

template <typename Key>
std::vector<DiceGroup*> sortedGroups(std::vector<DiceGroup>& groups, Key key) {
  std::vector<DiceGroup*> sorted;
  for (DiceGroup& group : groups) sorted.push_back(&group);
  std::stable_sort(sorted.begin(), sorted.end(), [&](DiceGroup* first, DiceGroup* second) { return key(*first) < key(*second); });
  return sorted;
}

}  // namespace

std::vector<DiceType> chooseDiceValue(const DiceRequirement& required, const std::vector<DiceType>& dice,
                                      const std::set<DiceType>& usefulDice, const std::set<DiceType>& targetDice,
                                      const std::set<DiceType>& disallowedDice) {
  std::vector<DiceType> result;
  int omniCount = disallowedDice.contains(dice::Omni) ? 0 : static_cast<int>(std::count(dice.begin(), dice.end(), dice::Omni));
  std::vector<DiceGroup> remaining = groupDice(dice, disallowedDice);

  // Elemental costs (types between Void and Aligned).
  for (const auto& [requiredType, requiredCount] : required.entries()) {
    if (!(requiredType > dice::Void && requiredType < dice::Aligned)) continue;
    auto target = std::find_if(remaining.begin(), remaining.end(), [&](const DiceGroup& group) { return group.type == requiredType; });
    if (target != remaining.end() && target->count + omniCount >= requiredCount) {
      int typedCount = std::min(target->count, requiredCount);
      int omniUsed = requiredCount - typedCount;
      result.insert(result.end(), typedCount, target->type);
      result.insert(result.end(), omniUsed, dice::Omni);
      target->count -= typedCount;
      omniCount -= omniUsed;
    } else if (omniCount >= requiredCount) {
      result.insert(result.end(), requiredCount, dice::Omni);
      omniCount -= requiredCount;
    } else {
      return {};
    }
  }

  if (auto alignedCount = required.find(dice::Aligned)) {
    int requiredCount = *alignedCount;
    std::vector<DiceGroup*> sorted = sortedGroups(remaining, [&](const DiceGroup& group) {
      return std::array<int, 5>{usefulDice.contains(group.type) ? 1 : 0, group.count >= requiredCount ? -1 : 0,
                                targetDice.contains(group.type) ? 1 : 0, std::abs(group.count - requiredCount), diceIndex(group.type)};
    });
    int minimumSame = std::max(0, requiredCount - omniCount);
    auto target = std::find_if(sorted.begin(), sorted.end(), [&](DiceGroup* group) { return group->count >= minimumSame; });
    if (target != sorted.end()) {
      int typedCount = std::min((*target)->count, requiredCount);
      int omniUsed = requiredCount - typedCount;
      result.insert(result.end(), typedCount, (*target)->type);
      result.insert(result.end(), omniUsed, dice::Omni);
      (*target)->count -= typedCount;
      omniCount -= omniUsed;
    } else if (omniCount >= requiredCount) {
      result.insert(result.end(), requiredCount, dice::Omni);
      omniCount -= requiredCount;
    } else {
      return {};
    }
  }

  if (auto voidCount = required.find(dice::Void)) {
    int requiredCount = *voidCount;
    std::vector<DiceGroup*> sorted = sortedGroups(remaining, [&](const DiceGroup& group) {
      return std::array<int, 4>{usefulDice.contains(group.type) ? 1 : 0, targetDice.contains(group.type) ? 1 : 0, group.count,
                                diceIndex(group.type)};
    });
    std::vector<DiceType> flat;
    for (DiceGroup* group : sorted) flat.insert(flat.end(), group->count, group->type);
    flat.insert(flat.end(), omniCount, dice::Omni);
    if (static_cast<int>(flat.size()) < requiredCount) return {};
    result.insert(result.end(), flat.begin(), flat.begin() + requiredCount);
  }
  return result;
}

bool checkDice(const DiceRequirement& required, const std::vector<DiceType>& chosen) {
  if (auto alignedCount = required.find(dice::Aligned)) {
    if (*alignedCount != static_cast<int>(chosen.size())) return false;
    std::set<DiceType> distinct(chosen.begin(), chosen.end());
    return (distinct.empty() && *alignedCount == 0) || distinct.size() == 1 || (distinct.size() == 2 && distinct.contains(dice::Omni));
  }
  std::vector<DiceType> left = chosen;
  int voidCount = 0;
  for (const auto& [type, count] : required.entries()) {
    for (int copy = 0; copy < count; ++copy) {
      if (type == dice::Energy || type == dice::Legend) continue;
      if (type == dice::Void) {
        ++voidCount;
        continue;
      }
      auto found = std::find(left.begin(), left.end(), type);
      if (found == left.end()) {
        auto omni = std::find(left.begin(), left.end(), dice::Omni);
        if (omni == left.end()) return false;
        left.erase(omni);
        continue;
      }
      left.erase(found);
    }
  }
  return static_cast<int>(left.size()) == voidCount;
}

}  // namespace gitcg
