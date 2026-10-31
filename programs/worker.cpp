// The C++ engine worker: a drop-in for reference/bridge/worker.mjs, one JSON message per line on
// stdin and stdout.

#include <cstdio>
#include <iostream>
#include <string>
#include <string_view>

#include "bridge/worker/worker.h"
#include "definitions/attributes/lambda_slots.h"
#include "definitions/registry/registry.h"

namespace gitcg {
void registerAllDefinitions(DefinitionRegistry& registry);
void bindTranslatedCardCode(LambdaSlots& slots);
void bindExtensionHooks(LambdaSlots& slots);
void bindHandPorts(LambdaSlots& slots);
namespace js {
void useCustomEventRegistry(const DefinitionRegistry* registry);
}
}  // namespace gitcg

int main(int argc, char** argv) {
  std::ios::sync_with_stdio(false);
  gitcg::DefinitionRegistry registry;
  gitcg::registerAllDefinitions(registry);
  gitcg::bindTranslatedCardCode(*registry.lambdaSlots());
  gitcg::bindExtensionHooks(*registry.lambdaSlots());
  gitcg::bindHandPorts(*registry.lambdaSlots());
  gitcg::js::useCustomEventRegistry(&registry);
  gitcg::GameData data = registry.resolve("v7.1.0");
  bool allowTestStates = argc == 2 && std::string_view(argv[1]) == "--test-snapshots";
  gitcg::Worker worker(data, allowTestStates);
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;
    try {
      if (!worker.handleLine(line)) break;
    } catch (const std::exception& error) {
      std::fprintf(stderr, "worker: %s\n", error.what());
    }
  }
  std::fflush(stdout);
  return 0;
}
