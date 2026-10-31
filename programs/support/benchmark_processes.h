// Twelve throughput processes: the parent runs shard 11 and waits after its own benchmark completes.
#pragma once
#include <string>
#include <vector>

namespace gitcg {
struct BenchmarkProcesses {
  int offset = 0;
  std::vector<int> children;
  bool finish() const;
};
BenchmarkProcesses launchBenchmarkProcesses(int games, int offset, const std::string& directory);
}  // namespace gitcg
