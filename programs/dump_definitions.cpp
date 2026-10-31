// Dump resolved definitions in exactly the same shape as tests/dump_definitions.mjs.
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include "definitions/verification/builtin_probe.h"
#include "core/skills/reaction_skills.h"
#include "definitions/registry/registry.h"
#include "definitions/registry/dump.h"
#include "core/json/json_text.h"

// This checkout declares reactionSkill but has no implementation yet. The dump never executes reactions.
// Keep this diagnostic-only guard out of libgitcg; it disappears when the expected core source is added.
#if !__has_include("core/skills/reaction_skills.cpp")
namespace gitcg {
const SkillDefinition* reactionSkill(Reaction) {
  throw std::runtime_error("core reactionSkill is not implemented in this checkout");
}
}
#endif

int main(int argc, char** argv) {
  try {
    if (argc > 2 && std::string(argv[1]) == "--builtins") {
      std::ifstream input(argv[2]);
      if (!input) throw std::runtime_error("Cannot open builtin fixture");
      std::ostringstream fixture; fixture << input.rdbuf();
      std::cout << gitcg::writeJson(gitcg::probeDefinitionBuiltins(fixture.str())) << '\n';
      return 0;
    }
    gitcg::DefinitionRegistry registry;
    gitcg::registerAllDefinitions(registry);
    auto data = registry.resolve(argc > 1 ? argv[1] : "v7.1.0");
    std::cout << gitcg::writeJson(gitcg::dumpDefinitions(data)) << '\n';
  } catch (const std::exception& error) {
    std::cerr << "dump_definitions: " << error.what() << '\n';
    return 1;
  }
}
