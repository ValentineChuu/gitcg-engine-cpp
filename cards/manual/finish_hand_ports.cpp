// One-off card-tail bindings. Sources and slot numbers refer to cards/generated/lambda_slots.json.
#include "cards/translated/translated_card_support.h"
#include "core/definitions/game_data.h"

namespace gitcg {

void bindFinishHandPorts(LambdaSlots& slots) {
  // other.gts:2892 Plan to Save the World: raw mutation deliberately bypasses heal/damage and variable events.
  slots.bind(826, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    for (const AnyHandle& character : c.queryAll(query::dollar().active())) {
      int direction = character.asCharacter().health() > 5 ? 2 : 1;
      c.mutateEntityVariable(character.id(), intern("health"), 5, direction);
    }
    return JsonValue();
  });
  // other.gts:2932 dispose the selected Simulanka summon, then strengthen all remaining ones.
  slots.bind(830, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    c.dispose(std::vector<AnyHandle>{c.get(c.e<InitiativeSkillArgument>().targets.at(0))});
    std::vector<query::Query> operands;
    for (int id : {301028, 301029, 301030, 301031}) operands.push_back(query::dollar().my().summon().def(id));
    for (const AnyHandle& summon : c.queryAll(query::dollar().union_(operands))) {
      summon.asEntity().addVariable(intern("effect"), 1);
      summon.asEntity().addVariable(intern("usage"), 1);
    }
    return JsonValue();
  });
  // adventure.gts:226 The Chasm checks the current version's registry before adding Adventure Completed.
  slots.bind(959, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    c.addVariable(intern("cardsPlayed"), 1);
    if (c.getVariable(intern("cardsPlayed")) >= 2) {
      if (auto chasm = c.query(query::dollar().my().support().def(321040))) c.dispose(std::vector<AnyHandle>{*chasm});
      c.generateDice(dice::Omni, 3);
      if (c.state().data->entities.find(171)) c.combatStatus(171);
      c.dispose();
    }
    return JsonValue();
  });
  // kaveh.gts:192 custom event payload is the discarded entity id; removed entities retain their definition.
  slots.bind(2209, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    const EntityDefinition& definition = c.get(static_cast<int>(c.e<CustomEventEventArg>().arg.asNumber())).asEntity().definition();
    c.createHandCard(definition.id);
    if (definition.hasTag(intern("place"))) c.combatStatus(117083 /* TheArtOfBudgetingInEffect */);
    return JsonValue();
  });
  // ororon.gts:217 replace the reaction core effect on its relative side, excluding the original target.
  slots.bind(2584, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto& event = c.e<ModifyReactionEventArg>();
    event.cancelCoreEffects();
    auto side = event.whereIsMine ? query::dollar().my() : query::dollar().opp();
    c.damage(damage::Piercing, 2, c.queryAll(side.character().exclude(query::dollar().id(event.target()->id))));
    return JsonValue();
  });
  // pyro_whopperflower.gts:102 only the first alive Whopperflower handles a disguise on the opponent's side.
  slots.bind(4650, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto& event = c.e<HandCardInsertedEventArg>();
    if (event.who == c.selfWho() || event.card->definition->id < 133085 || event.card->definition->id > 133099) return JsonValue(false);
    for (const CharacterPointer& character : c.player().characters) {
      if (character->definition->id == 3309 && character->variables.valueOr(intern("alive"), 0)) return JsonValue(character->id == c.selfCharacter().id());
    }
    return JsonValue(false);
  });
  // goldflame_qucusaur_tyrant.gts:21 one RNG draw even for an empty candidate list, preserving Map order.
  slots.bind(4882, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    c.characterStatus(123061 /* GoldflameState */, std::vector<AnyHandle>{c.self()});
    auto candidates = c.allCardDefinitions([](const EntityDefinition& card) {
      if (card.hasTag(intern("talent"))) return false;
      return std::any_of(card.skills.begin(), card.skills.end(), [](const auto& skill) {
        return skill->skillType == SkillType::PlayCard && skill->initiativeSkillConfig && skill->initiativeSkillConfig->diceCostSize == 2;
      });
    });
    auto chosen = js::randomOf(c, candidates);
    if (chosen) c.createHandCard((*chosen)->id);
    return JsonValue();
  });
}

}  // namespace gitcg
