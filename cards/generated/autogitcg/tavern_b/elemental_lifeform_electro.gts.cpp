// Generated from autogitcg/tavern_b/elemental_lifeform_electro.gts; AGPL-3.0. Run tools/translator/translate.mjs to regenerate.
#include "definitions/registry/registry.h"

namespace gitcg {
void register_f4f08b967333422f(DefinitionRegistry& registry) {
  registry.addModule(R"GTS([{"name":"status","positionals":[],"attributes":[{"name":"id","positionals":[134061],"attributes":[]},{"name":"variable","positionals":["enableModification",0],"attributes":[]},{"name":"on","positionals":["selfEnter"],"attributes":[{"name":"~action","positionals":[{"$lambda":4864}],"attributes":[]}]},{"name":"on","positionals":["modifyReaction"],"attributes":[{"name":"when","positionals":[{"$lambda":4865}],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4866}],"attributes":[]}]},{"name":"on","positionals":["decreaseDamaged"],"attributes":[{"name":"when","positionals":[{"$lambda":4867}],"attributes":[]},{"name":"~action","positionals":[{"$lambda":4868}],"attributes":[]}]}]},{"name":"skill","positionals":[],"attributes":[{"name":"id","positionals":[34063],"attributes":[]},{"name":"skillType","positionals":["passive"],"attributes":[{"name":"on","positionals":["battleBegin"],"attributes":[{"name":"~action","positionals":[{"$lambda":4869}],"attributes":[]}]}]}]}])GTS");
}
}  // namespace gitcg
