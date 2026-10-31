// Generated from autogitcg/weekly_b/electro_slime.gts; AGPL-3.0. Run tools/translator/translate.mjs to regenerate.
#include "definitions/registry/registry.h"

namespace gitcg {
void register_57d48649b8b53708(DefinitionRegistry& registry) {
  registry.addModule(R"GTS([{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[34061],"attributes":[]},{"name":"skillType","positionals":["normal"],"attributes":[]},{"name":"cost","positionals":[4,1],"attributes":[]},{"name":"cost","positionals":[0,2],"attributes":[]},{"name":"~action","positionals":[{"$lambda":5036}],"attributes":[]}]},{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[34062],"attributes":[]},{"name":"skillType","positionals":["burst"],"attributes":[]},{"name":"cost","positionals":[4,3],"attributes":[]},{"name":"cost","positionals":[9,2],"attributes":[]},{"name":"~action","positionals":[{"$lambda":5037}],"attributes":[]}]},{"name":"character","positionals":[],"attributes":[{"name":"id","positionals":[3406],"attributes":[]},{"name":"tags","positionals":["electro","monster"],"attributes":[]},{"name":"health","positionals":[4],"attributes":[]},{"name":"energy","positionals":[2],"attributes":[]},{"name":"skills","positionals":[34061,34062,34063],"attributes":[]}]}])GTS");
}
}  // namespace gitcg
