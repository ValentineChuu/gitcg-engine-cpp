// Test worker with self-generated crash traces for environments where ptrace/gdb is unavailable.
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <execinfo.h>
#include <iostream>
#include <unistd.h>
#include "bridge/worker/worker.h"
#include "definitions/attributes/lambda_slots.h"
#include "definitions/registry/registry.h"
namespace gitcg {
void registerAllDefinitions(DefinitionRegistry&);
void bindTranslatedCardCode(LambdaSlots&);
void bindExtensionHooks(LambdaSlots&);
}
namespace {
void crash(int signal) {
  void* frames[64]; int count = backtrace(frames, 64);
  backtrace_symbols_fd(frames, count, STDERR_FILENO);
  _Exit(128 + signal);
}
}
int main() {
  std::signal(SIGSEGV, crash); std::signal(SIGABRT, crash);
  gitcg::DefinitionRegistry registry; gitcg::registerAllDefinitions(registry);
  gitcg::bindTranslatedCardCode(*registry.lambdaSlots()); gitcg::bindExtensionHooks(*registry.lambdaSlots());
  auto data = registry.resolve("v7.1.0"); gitcg::Worker worker(data, true);
  std::string line;
  while (std::getline(std::cin, line)) if (!line.empty() && !worker.handleLine(line)) break;
}
