// Generated from autogitcg/open_world_b/pyro_abyss_mage_open_world.gts; AGPL-3.0. Run tools/translator/translate.mjs to regenerate.
#include "definitions/registry/registry.h"

namespace gitcg {
void register_61b83bb46c8acd21(DefinitionRegistry& registry) {
  registry.addModule(R"GTS([{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[33016],"attributes":[]},{"name":"skillType","positionals":["passive"],"attributes":[{"name":"on","positionals":["useSkill"],"attributes":[{"name":"when","positionals":[{"$lambda":4734}],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4735}],"attributes":[]}]}]}]},{"name":"character","positionals":[],"attributes":[{"name":"id","positionals":[5301],"attributes":[]},{"name":"tags","positionals":["pyro","monster"],"attributes":[]},{"name":"health","positionals":[6],"attributes":[]},{"name":"energy","positionals":[3],"attributes":[]},{"name":"skills","positionals":[33011,33012,33013,33014,33016],"attributes":[]}]}])GTS");
}
}  // namespace gitcg
