// Generated from autogitcg/open_world_b/hydro_abyss_mage_open_world.gts; AGPL-3.0. Run tools/translator/translate.mjs to regenerate.
#include "definitions/registry/registry.h"

namespace gitcg {
void register_e1582411cf20b60a(DefinitionRegistry& registry) {
  registry.addModule(R"GTS([{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[32016],"attributes":[]},{"name":"skillType","positionals":["passive"],"attributes":[{"name":"on","positionals":["useSkill"],"attributes":[{"name":"when","positionals":[{"$lambda":4706}],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4707}],"attributes":[]}]}]}]},{"name":"character","positionals":[],"attributes":[{"name":"id","positionals":[5201],"attributes":[]},{"name":"tags","positionals":["hydro","monster"],"attributes":[]},{"name":"health","positionals":[6],"attributes":[]},{"name":"energy","positionals":[3],"attributes":[]},{"name":"skills","positionals":[32011,32012,32013,32014,32016],"attributes":[]}]}])GTS");
}
}  // namespace gitcg
