// Elemental blessings auto-play from hand/pile, discard duplicates and convert dice in source order.
#include "definitions/entities/builtins.h"
#include "core/rules/character_traits.h"
#include "core/rules/entity_lookup.h"

namespace gitcg {
void addElementalBlessing(EntityModel& model, const Attribute& attribute) {
  model.definition.obtainable = false;
  model.definition.tags.push_back(intern("blessing"));
  int type1 = attribute.argument(0).asInt(), type2 = attribute.argument(1).asInt();
  model.addBuiltin("actionPhase", [type1, type2](SkillContext& c) {
    auto self = c.selfEntity();
    int definitionId = self.definition().id;
    if (self.area().type == AreaType::Pile) c.drawCards(std::vector<EntityHandle>{self});
    if (self.area().type != AreaType::Hands) return;
    std::vector<EntityHandle> duplicates;
    for (const auto& entry : getAllEntities(c.state())) {
      if (entry.area.who == c.selfWho() && entry.state.isEntity() &&
          (entry.area.type == AreaType::Hands || entry.area.type == AreaType::Pile) &&
          entry.state.entity()->definition->id == definitionId) duplicates.emplace_back(&c, entry.state.id());
    }
    for (const auto& card : duplicates) c.discard({card});
    c.createEntity(EntityType::Support, definitionId, EntityArea{AreaType::Supports, c.selfWho()});
    c.convertDice(type1, 2); c.convertDice(type2, 2);
  }, [type1, type2](SkillContext& c) {
    if (c.selfEntity().area().type == AreaType::Supports) return false;
    std::vector<int> elements;
    for (const auto& ch : c.player().characters) {
      int element = CharacterHandle(&c, ch->id).element();
      if (std::find(elements.begin(), elements.end(), element) == elements.end()) elements.push_back(element);
    }
    return elements.size() == 2 && std::find(elements.begin(), elements.end(), type1) != elements.end() && std::find(elements.begin(), elements.end(), type2) != elements.end();
  }, "sameArea", false, true, true);
}
}  // namespace gitcg
