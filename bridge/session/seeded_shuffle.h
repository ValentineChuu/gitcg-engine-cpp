// The bridge's deck shuffle (reference/bridge/seeded_random.mjs): an LCG computed in doubles exactly as JS does
// (the product can pass 2^53 and round), and a Fisher-Yates shuffle from the last index down.
#pragma once

#include <cmath>
#include <vector>

namespace gitcg {

class SeededRandom {
 public:
  explicit SeededRandom(double seed) : value_(std::fmod(seed, 2147483648.0)) {}
  double next() {
    value_ = std::fmod(value_ * 1103515245.0 + 12345.0, 2147483648.0);
    return value_ / 2147483648.0;
  }

 private:
  double value_;
};

template <typename Item>
std::vector<Item> seededShuffle(std::vector<Item> items, SeededRandom& random) {
  for (size_t index = items.size(); index-- > 1;) {
    size_t swapIndex = static_cast<size_t>(std::floor(random.next() * static_cast<double>(index + 1)));
    std::swap(items[index], items[swapIndex]);
  }
  return items;
}

}  // namespace gitcg
