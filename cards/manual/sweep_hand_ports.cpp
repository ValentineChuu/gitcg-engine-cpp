// Remaining synthetic-sweep and wide-game card lambdas, with their original JS operation order.
#include "cards/translated/translated_card_support.h"

namespace gitcg {

void bindSweepHandPorts(LambdaSlots& slots) {
  // legend.gts:516 discard three highest-cost cards, then store their definitions in reverse original hand order.
  slots.bind(921, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    std::vector<int> originalIds;
    for (const EntityPointer& card : c.player().hands) originalIds.push_back(card->id);
    auto cards = c.discardMaxCostHands(3);
    auto position = [&](int id) { return std::find(originalIds.begin(), originalIds.end(), id) - originalIds.begin(); };
    std::stable_sort(cards.begin(), cards.end(), [&](const EntityHandle& a, const EntityHandle& b) {
      return position(a.id()) > position(b.id());
    });
    const char* names[] = {"card0Id", "card1Id", "card2Id"};
    for (size_t index = 0; index < cards.size() && index < 3; ++index) c.setVariable(intern(names[index]), cards[index].definition().id);
    return JsonValue();
  });
  // legend.gts:534 recreate the stored cards, attach three cost reduction to each successful creation, then dispose.
  slots.bind(925, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    const std::vector<double> ids{c.getVariable(intern("card0Id")), c.getVariable(intern("card1Id")), c.getVariable(intern("card2Id"))};
    for (double id : ids) {
      if (!js::truthy(id)) continue;
      if (auto card = c.createHandCard(id)) c.attachCostReduction(std::vector<AnyHandle>{js::handle(c, *card)}, 3);
    }
    c.dispose();
    return JsonValue();
  });
  // pyro_whopperflower.gts:35 two real cards, one disguise, then one random insertion position (four RNG draws).
  slots.bind(4644, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto realCards = c.randomSubset(c.allCardDefinitions([](const EntityDefinition& card) {
      return (card.type == EntityType::EventCard || card.type == EntityType::Support || card.type == EntityType::Equipment) &&
             !card.hasTag(intern("talent"));
    }), 2);
    std::vector<double> disguises;
    for (int id = 133085; id <= 133099; ++id) disguises.push_back(id);
    double disguise = *js::randomOf(c, disguises);
    std::vector<int> offer;
    for (const EntityDefinition* card : realCards) offer.push_back(card->id);
    int position = static_cast<int>(*js::randomOf(c, std::vector<double>{0, 1, 2}));
    offer.insert(offer.begin() + std::min<size_t>(position, offer.size()), static_cast<int>(disguise));
    c.selectAndCreateHandCard(offer);
    return JsonValue();
  });
  // tavern_b/lord_of_eroded_primal_fire.gts:24 copy each player's first nine pile cards, then discard in reverse.
  slots.bind(4886, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    c.abortPreview();
    for (bool opponent : {false, true}) {
      auto cards = js::slice(js::entityHandles(c, opponent ? c.oppPlayer().pile : c.player().pile), 0, 9);
      for (auto card = cards.rbegin(); card != cards.rend(); ++card) c.discard(std::vector<EntityHandle>{*card});
    }
    return JsonValue();
  });
}

}  // namespace gitcg
