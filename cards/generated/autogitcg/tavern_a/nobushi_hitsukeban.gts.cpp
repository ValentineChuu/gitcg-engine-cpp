// Generated from autogitcg/tavern_a/nobushi_hitsukeban.gts; AGPL-3.0. Run tools/translator/translate.mjs to regenerate.
#include "definitions/registry/registry.h"

namespace gitcg {
void register_1c6588a08580526f(DefinitionRegistry& registry) {
  registry.addModule(R"GTS([{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[33051],"attributes":[]},{"name":"skillType","positionals":["normal"],"attributes":[]},{"name":"cost","positionals":[3,1],"attributes":[]},{"name":"cost","positionals":[0,2],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4846}],"attributes":[]}]},{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[33052],"attributes":[]},{"name":"skillType","positionals":["elemental"],"attributes":[]},{"name":"cost","positionals":[3,3],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4847}],"attributes":[]}]},{"name":"character","positionals":[],"attributes":[{"name":"id","positionals":[3305],"attributes":[]},{"name":"tags","positionals":["pyro"],"attributes":[]},{"name":"health","positionals":[6],"attributes":[]},{"name":"energy","positionals":[2],"attributes":[]},{"name":"skills","positionals":[33051,33052,30033,30034,30035,30036],"attributes":[]}]}])GTS");
}
}  // namespace gitcg
