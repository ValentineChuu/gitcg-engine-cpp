// Persistent root copies, kept alive in batches so the optimizer cannot erase reference-count operations.
#include <chrono>
#include <cstdio>
#include <vector>
#include "core/state/game_state.h"
using namespace gitcg;
int main(int argc, char** argv) {
  int64_t count = argc > 1 ? std::stoll(argv[1]) : 10000000;
  auto state = makeRef<GameState>();
  state->players[0] = makeRef<PlayerState>(); state->players[1] = makeRef<PlayerState>();
  std::vector<GameStatePointer> copies; copies.reserve(10000);
  auto started = std::chrono::steady_clock::now();
  int64_t checked = 0;
  for (int64_t i = 0; i < count; ++i) {
    copies.push_back(state);
    if (copies.size() == 10000) { for (const auto& copy : copies) checked += copy->currentTurn + 1; copies.clear(); }
  }
  for (const auto& copy : copies) checked += copy->currentTurn + 1;
  copies.clear();
  double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
  std::printf("{\"copies\":%lld,\"checked\":%lld,\"seconds\":%.6f,\"copiesPerSecond\":%.0f}\n",
    static_cast<long long>(count), static_cast<long long>(checked), seconds, count / seconds);
  return checked == count ? 0 : 1;
}
