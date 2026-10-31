#include "programs/support/benchmark_processes.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

namespace gitcg {
namespace {
void redirectOutput(const std::string& directory, int index) {
  std::string path = directory + "/" + std::to_string(index) + ".log";
  if (!std::freopen(path.c_str(), "w", stdout) || !std::freopen(path.c_str(), "a", stderr)) {
    std::perror("benchmark output");
    std::exit(2);
  }
}
}  // namespace

BenchmarkProcesses launchBenchmarkProcesses(int games, int offset, const std::string& directory) {
  BenchmarkProcesses result{.offset = offset + 11 * games, .children = {}};
  for (int index = 0; index < 11; ++index) {
    int child = fork();
    if (child < 0) {
      result.finish();
      throw std::runtime_error("Cannot fork benchmark child");
    }
    if (child == 0) {
      redirectOutput(directory, index);
      return {.offset = offset + index * games, .children = {}};
    }
    result.children.push_back(child);
  }
  redirectOutput(directory, 11);
  return result;
}

bool BenchmarkProcesses::finish() const {
  bool passed = true;
  for (int child : children) {
    int status = 0;
    int waited;
    do { waited = waitpid(child, &status, 0); } while (waited < 0 && errno == EINTR);
    if (waited < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) passed = false;
  }
  return passed;
}
}  // namespace gitcg
