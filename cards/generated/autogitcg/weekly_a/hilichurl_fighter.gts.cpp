// Generated from autogitcg/weekly_a/hilichurl_fighter.gts; AGPL-3.0. Run tools/translator/translate.mjs to regenerate.
#include "definitions/registry/registry.h"

namespace gitcg {
void register_b77ef2ca0bc1d9bf(DefinitionRegistry& registry) {
  registry.addModule(R"GTS([{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[30011],"attributes":[]},{"name":"skillType","positionals":["normal"],"attributes":[]},{"name":"cost","positionals":[0,2],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4942}],"attributes":[]}]},{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[30012],"attributes":[]},{"name":"skillType","positionals":["elemental"],"attributes":[]},{"name":"cost","positionals":[0,3],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4943}],"attributes":[]}]},{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[30013],"attributes":[]},{"name":"skillType","positionals":["burst"],"attributes":[]},{"name":"cost","positionals":[8,3],"attributes":[]},{"name":"cost","positionals":[9,2],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4944}],"attributes":[]}]},{"name":"character","positionals":[],"attributes":[{"name":"id","positionals":[3001],"attributes":[]},{"name":"tags","positionals":["monster","hilichurl"],"attributes":[]},{"name":"health","positionals":[4],"attributes":[]},{"name":"energy","positionals":[2],"attributes":[]},{"name":"skills","positionals":[30011,30012,30013],"attributes":[]}]}])GTS");
}
}  // namespace gitcg
