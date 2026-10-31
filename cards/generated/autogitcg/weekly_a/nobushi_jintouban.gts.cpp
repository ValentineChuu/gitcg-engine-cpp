// Generated from autogitcg/weekly_a/nobushi_jintouban.gts; AGPL-3.0. Run tools/translator/translate.mjs to regenerate.
#include "definitions/registry/registry.h"

namespace gitcg {
void register_19084eed6500f492(DefinitionRegistry& registry) {
  registry.addModule(R"GTS([{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[30031],"attributes":[]},{"name":"skillType","positionals":["normal"],"attributes":[]},{"name":"cost","positionals":[0,3],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4970}],"attributes":[]}]},{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[30032],"attributes":[]},{"name":"skillType","positionals":["elemental"],"attributes":[]},{"name":"cost","positionals":[8,3],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4971}],"attributes":[]}]},{"name":"character","positionals":[],"attributes":[{"name":"id","positionals":[3003],"attributes":[]},{"name":"health","positionals":[6],"attributes":[]},{"name":"energy","positionals":[2],"attributes":[]},{"name":"skills","positionals":[30031,30032,30033,30034,30035,30036],"attributes":[]}]}])GTS");
}
}  // namespace gitcg
