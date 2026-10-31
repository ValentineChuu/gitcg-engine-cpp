// The core's "minstd" generator (core/src/random.ts): x -> 48271 x mod (2^31 - 1).
#pragma once

#include <cstdint>

namespace gitcg {

inline int64_t nextRandom(int64_t value) {
  return (48271 * value) % 2147483647;
}

}  // namespace gitcg
