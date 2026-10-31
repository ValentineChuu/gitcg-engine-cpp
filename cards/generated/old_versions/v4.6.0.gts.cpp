// Generated from old_versions/v4.6.0.gts; AGPL-3.0. Run tools/translator/translate.mjs to regenerate.
#include "definitions/registry/registry.h"

namespace gitcg {
void register_afb0adf56ae5e0a8(DefinitionRegistry& registry) {
  registry.addModule(R"GTS([{"name":"card","positionals":[],"attributes":[{"name":"id","positionals":[332031],"attributes":[]},{"name":"until","positionals":["v4.6.0"],"attributes":[]},{"name":"cost","positionals":[8,1],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4089}],"attributes":[]}]},{"name":"combatStatus","positionals":[],"attributes":[{"name":"id","positionals":[112092],"attributes":[]},{"name":"until","positionals":["v4.6.0"],"attributes":[]},{"name":"duration","positionals":[2],"attributes":[]},{"name":"on","positionals":["useSkill"],"attributes":[{"name":"when","positionals":[{"$lambda":4090}],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4091}],"attributes":[]}]}]},{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[12093],"attributes":[]},{"name":"until","positionals":["v4.6.0"],"attributes":[]},{"name":"skillType","positionals":["burst"],"attributes":[]},{"name":"cost","positionals":[2,3],"attributes":[]},{"name":"cost","positionals":[9,3],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4092}],"attributes":[]}]}])GTS");
}
}  // namespace gitcg
