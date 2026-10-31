// Generated from autogitcg/weekly_a/hilichurl_shooter.gts; AGPL-3.0. Run tools/translator/translate.mjs to regenerate.
#include "definitions/registry/registry.h"

namespace gitcg {
void register_f34d6f9b627c0b07(DefinitionRegistry& registry) {
  registry.addModule(R"GTS([{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[30021],"attributes":[]},{"name":"skillType","positionals":["normal"],"attributes":[]},{"name":"cost","positionals":[0,2],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4945}],"attributes":[]}]},{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[30022],"attributes":[]},{"name":"skillType","positionals":["burst"],"attributes":[]},{"name":"cost","positionals":[8,3],"attributes":[]},{"name":"cost","positionals":[9,2],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4946}],"attributes":[]}]},{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[30023],"attributes":[]},{"name":"skillType","positionals":["passive"],"attributes":[{"name":"on","positionals":["useSkill"],"attributes":[{"name":"~action","positionals":[{"$lambda":4947}],"attributes":[]}]}]}]},{"name":"character","positionals":[],"attributes":[{"name":"id","positionals":[3002],"attributes":[]},{"name":"tags","positionals":["monster","hilichurl"],"attributes":[]},{"name":"health","positionals":[3],"attributes":[]},{"name":"energy","positionals":[2],"attributes":[]},{"name":"skills","positionals":[30021,30022,30023],"attributes":[]}]}])GTS");
}
}  // namespace gitcg
