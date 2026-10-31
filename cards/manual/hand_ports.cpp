// Hand ports of card lambdas the translator doesn't handle (inner functions, destructuring, Object.keys, module
// constants, ...). Each binding names its slot and source line; the JS is in cards/generated/lambda_slots.json.
// Bound after the translated code, so a hand port wins over a translation of the same slot.

#include <algorithm>
#include <map>
#include <string>

#include "cards/translated/translated_card_support.h"
#include "core/definitions/game_data.h"
#include "core/rules/costs.h"

namespace gitcg {

void bindFinishHandPorts(LambdaSlots& slots);
void bindSweepHandPorts(LambdaSlots& slots);

namespace {

const Name& variable(const char* text) {
  static thread_local std::map<std::string, Name> names;
  auto found = names.find(text);
  if (found == names.end()) found = names.emplace(text, intern(text)).first;
  return found->second;
}

}  // namespace

void bindHandPorts(LambdaSlots& slots) {
  // other.gts:1363 Lyresong: unequip the target's artifact; the stronger effect when it is the first card this round.
  slots.bind(684, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    CharacterHandle(&c, c.e<InitiativeSkillArgument>().targets.at(0).id()).unequipArtifact();
    bool first = c.getExtensionState().at("first").asArray().at(static_cast<size_t>(c.selfWho())).asBool();
    c.combatStatus(first ? 303224 /* LyresongInEffect2 */ : 303232 /* LyresongInEffect1 */);
    return JsonValue();
  });
  // place.gts:435 filter: my equipment's original dice cost >= the opponent's.
  slots.bind(1262, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto total = [&](const query::Query& query) {
      double sum = 0;
      for (const AnyHandle& entity : c.queryAll(query)) sum += originalDiceCostSizeOfCard(entity.asEntity().definition());
      return sum;
    };
    return JsonValue(total(query::dollar().my().equipped()) >= total(query::dollar().opp().equipped()));
  });
  // alldevouring_narwhal.gts:137 record the dice cost of each card in groups of three, and the highest cost seen.
  slots.bind(3005, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    double cost = c.e<DisposeEventArg>().diceCost();
    c.addVariable(variable("cardCount"), 1);
    double cardCount = c.getVariable(variable("cardCount"));
    if (cardCount == 1) {
      c.setVariable(variable("card0Cost"), cost);
    } else if (cardCount == 2) {
      c.setVariable(variable("card1Cost"), cost);
    } else if (cardCount == 3) {
      std::vector<double> costs{c.getVariable(variable("card0Cost")), c.getVariable(variable("card1Cost")), cost};
      double distinctCostCount = static_cast<double>(js::distinct(costs).size());
      c.addVariable(variable("extraMaxHealth"), 4 - distinctCostCount);
      c.setVariable(variable("cardCount"), 0);
    }
    double previousTotalMaxCost = c.getVariable(variable("totalMaxCost"));
    if (cost == previousTotalMaxCost) {
      c.addVariable(variable("totalMaxCostCount"), 1);
    } else if (cost > previousTotalMaxCost) {
      c.setVariable(variable("totalMaxCost"), cost);
      c.setVariable(variable("totalMaxCostCount"), 1);
    }
    return JsonValue();
  });
  // keqing.gts:82 Stellar Restoration: infuse (via the card or by playing it from hand), else create the card.
  slots.bind(2515, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    const int LIGHTNING_STILETTO = 114031;
    c.damage(damage::Electro, 3);
    const auto& requestBy = c.skillInfo().requestBy;
    bool requestByCard = requestBy && !requestBy->caller.empty() &&
                         (requestBy->caller.isCharacter() ? requestBy->caller.character()->definition->id
                                                          : requestBy->caller.entity()->definition->id) == LIGHTNING_STILETTO;
    std::optional<EntityHandle> card;
    for (const EntityPointer& hand : c.player().hands) {
      if (hand->definition->id == LIGHTNING_STILETTO) {
        card = EntityHandle(&c, hand->id);
        break;
      }
    }
    if (requestByCard || card) {
      if (c.selfCharacter().hasEquipment(214031 /* ThunderingPenance */)) {
        c.characterStatus(114034 /* ElectroElementalInfusion01 */);
      } else {
        c.characterStatus(114032 /* ElectroElementalInfusion */);
      }
      if (card) c.discard({*card});
    } else {
      c.createHandCard(LIGHTNING_STILETTO);
    }
    return JsonValue();
  });
  // chiori.gts:229 summon Tamoto with the talent, then select from 3 (4) random useful dolls.
  slots.bind(2782, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    int count = 3;
    if (c.selfCharacter().hasEquipment(216091 /* InFiveColorsDyed */)) {
      c.summon(116094 /* TranquillyTakingTenTamoto */);
      count = 4;
    }
    std::vector<int> usefulDolls{116091, 116092, 116093, 116095, 116096};
    c.selectAndSummon(c.randomSubset(usefulDolls, count));
    return JsonValue();
  });
  // chongyun.gts:67 filter: physical damage from a sword, claymore or pole character.
  slots.bind(1714, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto& e = c.e<DamageOrHealEventArg>();
    if (e.type() != damage::Physical) return JsonValue(false);
    AnyState source = c.get(e.source()).latest();
    const std::vector<Name>& tags = source.isCharacter() ? source.character()->definition->tags : source.entity()->definition->tags;
    return JsonValue(js::hasTag(tags, "sword") || js::hasTag(tags, "claymore") || js::hasTag(tags, "pole"));
  });
  // other.gts:1016 targets: my other characters.
  slots.bind(637, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    int target = c.e<InitiativeSkillArgument>().targets.at(0).id();
    return js::ids(c.queryAll(query::dollar().my().character().exclude(query::dollar().id(target))));
  });
  // other.gts:985 targets: my other characters using the same weapon type.
  slots.bind(631, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    int target = c.e<InitiativeSkillArgument>().targets.at(0).id();
    return js::ids(c.queryAll(query::dollar().my().character().tagOf("weapon", query::dollar().id(target)).exclude(query::dollar().id(target))));
  });
  // other.gts:965 move energy from up to two standby characters to the active one.
  slots.bind(626, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    double count = 0;
    for (const AnyHandle& character : c.queryAll(query::dollar().my().standby().limit(2))) count += character.asCharacter().loseEnergy();
    if (std::optional<AnyHandle> active = c.query(query::dollar().my().active())) active->asCharacter().gainEnergy(count);
    return JsonValue();
  });
  // ally.gts:404 one omni die; then move this support to the opponent's side if there is room.
  slots.bind(1004, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    c.generateDice(dice::Omni, 1);
    if (c.remainingSupportCount(true) > 0) c.moveEntity(c.selfEntity(), EntityArea{AreaType::Supports, 1 - c.selfWho(), 0, 0});
    return JsonValue();
  });
  // techniques.gts:70 steal the opponent's most expensive hand card, then they draw one.
  slots.bind(221, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    c.damage(damage::Physical, 1);
    std::vector<EntityHandle> cards = c.maxCostHands(1, true);
    if (!cards.empty()) c.stealHandCard(cards.front());
    c.drawCards(1, true);
    return JsonValue();
  });
  // citlali.gts:72 filter: Citlali's Nightsoul's Blessing at 2 nightsoul.
  slots.bind(1724, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    AnyHandle entity = c.get(c.e<EntityEventArg>().entity);
    if (entity.isCharacter() || entity.asEntity().definition().id != 111141 /* NightsoulsBlessing */) return JsonValue(false);
    return JsonValue(entity.asEntity().getVariable(variable("nightsoul")) == 2);
  });
  // other.gts:315 filter: a reaction from my damage involving Anemo.
  slots.bind(545, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto& e = c.e<ReactionEventArg>();
    const auto& fromDamage = e.reactionInfo.fromDamage;
    if (!fromDamage) return JsonValue(false);
    if (c.entityArea(fromDamage->source.id()).who != c.selfWho()) return JsonValue(false);
    return JsonValue(e.relatedTo(damage::Anemo));
  });
  // sayu.gts:97 Fuufuu Windwheel takes the element the opponent's active character's aura would swirl.
  slots.bind(1583, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    std::optional<AnyHandle> active = c.query(query::dollar().opp().active());
    DamageType windType = damage::Anemo;
    if (active) {
      switch (active->asCharacter().aura()) {
        case aura::Cryo:
        case aura::CryoDendro: windType = damage::Cryo; break;
        case aura::Hydro: windType = damage::Hydro; break;
        case aura::Pyro: windType = damage::Pyro; break;
        case aura::Electro: windType = damage::Electro; break;
        default: break;
      }
    }
    c.characterStatus(115071 /* FuufuuWindwheel */, {c.self()}, CreateEntityOptions{.overrideVariables = {{variable("swirled"), static_cast<double>(windType)}}});
    c.damage(damage::Anemo, 1);
    return JsonValue();
  });
  // other.gts:1018 move the first target's artifact (per-round usages reset) to the second target.
  slots.bind(640, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    const auto& targets = c.e<InitiativeSkillArgument>().targets;
    EntityHandle artifact = *CharacterHandle(&c, targets.at(0).id()).hasArtifact();
    artifact.resetUsagePerRound();
    CharacterHandle target(&c, targets.at(1).id());
    EntityArea area{AreaType::Characters, target.who(), target.id(), 0};
    if (std::optional<EntityHandle> old = target.hasArtifact()) c.dispose(std::vector<AnyHandle>{AnyHandle(&c, old->id(), false)});
    c.moveEntity(artifact, area);
    return JsonValue();
  });
  // navia.gts:95 Geo infusion; damage and redraw per Crystal Shrapnel in hand (at most 5), which are discarded.
  slots.bind(2865, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    c.characterStatus(116084 /* GeoInfusion */);
    std::vector<EntityHandle> shrapnels;
    for (const EntityPointer& card : c.player().hands) {
      if (card->definition->id == 116081 /* CrystalShrapnel */ && shrapnels.size() < 5) shrapnels.emplace_back(&c, card->id);
    }
    double count = static_cast<double>(shrapnels.size());
    c.damage(damage::Geo, 3 + count);
    c.discard(shrapnels);
    c.drawCards(static_cast<int>(count));
    return JsonValue();
  });
  // xilonen.gts:303 source samples: 3 Geo layers, one moved to each other element among my characters.
  slots.bind(2933, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    // Object.keys of {Geo, Hydro, Pyro, Cryo, Electro}: integer keys in ascending order (Cryo 1 ... Geo 6).
    const std::vector<std::pair<DiceType, int>> sampleMap = {
        {dice::Cryo, 116116}, {dice::Hydro, 116114}, {dice::Pyro, 116115}, {dice::Electro, 116117}, {dice::Geo, 116113}};
    std::map<DiceType, double> sampleCount;
    for (const auto& [element, status] : sampleMap) sampleCount[element] = 0;
    sampleCount[dice::Geo] = 3;
    std::vector<double> elements;
    for (const AnyHandle& character : c.queryAll(query::dollar().my().character().includesDefeated())) {
      elements.push_back(character.asCharacter().element());
    }
    for (double element : js::distinct(elements)) {
      auto type = static_cast<DiceType>(element);
      if (type != dice::Geo && sampleCount.contains(type)) {
        ++sampleCount[type];
        --sampleCount[dice::Geo];
      }
    }
    for (const auto& [element, status] : sampleMap) {
      double layer = sampleCount[element];
      if (layer > 0) c.characterStatus(status, {c.self()}, CreateEntityOptions{.overrideVariables = {{variable("layer"), layer}}});
    }
    return JsonValue();
  });
  // clorinde.gts:32 filter: heals not from Hunter's Vigil (14122).
  slots.bind(2369, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    return JsonValue(c.e<ModifyHealEventArg>().via()->definition->id != 14122);
  });
  // item.gts:200 filter: an equipment/place/ally card this player has played before.
  slots.bind(1211, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto& e = c.e<ActionEventArg>();
    if (!e.hasOneOfCardTag({intern("weapon"), intern("artifact"), intern("place"), intern("ally")})) return JsonValue(false);
    double id = e.action().skill->caller.entity()->definition->id;
    for (const JsonValue& played : c.getExtensionState().at("played").asArray().at(static_cast<size_t>(c.selfWho())).asArray()) {
      if (played.asNumber() == id) return JsonValue(true);
    }
    return JsonValue(false);
  });
  // ally.gts:925 experience = min(supports this player has disposed, 6).
  slots.bind(1033, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    double disposed = c.getExtensionState().at("disposedSupportCount").asArray().at(static_cast<size_t>(c.selfWho())).asNumber();
    c.setVariable(variable("experience"), std::min(disposed, 6.0));
    return JsonValue();
  });
  // commons.gts BondOfLife: `on damaged` passes the damage event itself to BondOfLifeOnDamaged listeners.
  slots.bind(13, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    c.handleCustomEventInline(js::customEvent("commons.gts:24900"), JsonValue(), &c.eventArgument());
    return JsonValue();
  });
  // arlecchino.gts: BondOfLifeOnDamaged from an opponent's status, caused by Invitation to a Beheading.
  slots.bind(3452, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    const auto& event = c.e<CustomEventEventArg>();
    const auto& damaged = static_cast<const DamageOrHealEventArg&>(*event.rawArg);
    return JsonValue(!js::isMine(c.get(event.entity)) && damaged.via()->definition->id == 13141 /* InvitationToABeheading */);
  });
  // commons.gts BondOfLife on decreaseHealed: not for distributed health.
  slots.bind(10, [](LambdaInvocation& call) -> JsonValue {
    return JsonValue(call.context->e<ModifyHealEventArg>().healInfo().healKind != HealKind::Distribution);
  });
  // azhdaha.gts: the transformation changed the definition.
  slots.bind(2721, [](LambdaInvocation& call) -> JsonValue {
    const auto& event = call.context->e<TransformDefinitionEventArg>();
    return JsonValue(event.oldDefinition.id() != event.newDefinition.id());
  });
  // mirror_maiden.gts: a switch away from the status's master.
  slots.bind(3235, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    ActionInfo action = c.e<ModifyActionEventArg>().action();
    return JsonValue(action.type == ActionType::SwitchActive && action.from && c.selfEntity().master().id() == action.from->id);
  });
  // commons.gts BondOfLife on decreaseHealed: absorb healing up to its usage.
  slots.bind(11, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto& event = c.e<ModifyHealEventArg>();
    double deducted = std::min(c.getVariable(intern("usage")), event.expectedValue());
    event.decreaseHeal(deducted);
    c.consumeUsage(deducted);
    return JsonValue();
  });
  // baizhu.gts: entering replaced an existing instance.
  slots.bind(2074, [](LambdaInvocation& call) -> JsonValue {
    return JsonValue(call.context->e<EnterEventArg>().overridden != nullptr);
  });
  // place.gts: the layer reached 3 on this entry.
  slots.bind(1304, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    const EntityPointer& overridden = c.e<EnterEventArg>().overridden;
    double previous = overridden ? overridden->variables.valueOr(intern("layer"), 0) : 0;
    return JsonValue(previous < 3 && c.getVariable(intern("layer")) >= 3);
  });
  // jadeplume_terrorshroom.gts: the addVitality snippet (`:get(:e)` is the master), inlined at both call sites.
  for (int slot : {2177, 2179}) {
    slots.bind(slot, [](LambdaInvocation& call) -> JsonValue {
      SkillContext& c = *call.context;
      double max = c.selfEntity().master().hasEquipment(227011 /* ProliferatingSpores */).has_value() ? 4 : 3;
      if (c.getVariable(intern("vitality")) < max) c.addVariable(intern("vitality"), 1);
      return JsonValue();
    });
  }
  // azhdaha.gts: remember the absorbed element once per player and gain one die of it.
  slots.bind(2722, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    static const std::map<int, DiceType> diceOfForm = {
        {6601, dice::Cryo} /* AzhdahaCryo */, {6602, dice::Hydro} /* AzhdahaHydro */,
        {6603, dice::Pyro} /* AzhdahaPyro */, {6604, dice::Electro} /* AzhdahaElectro */};
    auto form = diceOfForm.find(c.selfEntity().master().definition().id);
    DiceType diceType = form == diceOfForm.end() ? dice::Geo : form->second;
    int who = c.selfEntity().who();
    c.setExtensionState([&](JsonValue& state) {
      JsonValue& absorbed = state["absorbed"].asArray().at(static_cast<size_t>(who));
      if (std::find(absorbed.asArray().begin(), absorbed.asArray().end(), JsonValue(static_cast<double>(diceType))) == absorbed.asArray().end()) {
        absorbed.push(JsonValue(static_cast<double>(diceType)));
      }
    });
    c.generateDice(diceType, 1);
    return JsonValue();
  });
  // other.gts: two random legend cards, then both players may play a legend again.
  slots.bind(737, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto candidates = c.allCardDefinitions([](const EntityDefinition& definition) { return definition.hasTag(intern("legend")); });
    auto card0 = js::randomOf(c, candidates);
    auto card1 = js::randomOf(c, candidates);
    c.createHandCard((*card0)->id);
    c.createHandCard((*card1)->id);
    c.setPlayerFlag(0, PlayerFlag::LegendUsed, false);
    c.setPlayerFlag(1, PlayerFlag::LegendUsed, false);
    return JsonValue();
  });
  // legend.gts: an opponent's character fell on my turn while the action phase (or a defeat switch) is on.
  slots.bind(881, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    bool targetIsMine = c.entityArea(c.e<DamageOrHealEventArg>().target()->id).who == c.selfWho();
    return JsonValue(c.isMyTurn() && !c.oppPlayer().declaredEnd && !targetIsMine &&
                     (c.phase() == Phase::Action || c.player().defeatedSwitching || c.oppPlayer().defeatedSwitching));
  });
  // rosaria.gts: a layer-in-10 chance to take the status, then halve the layers.
  slots.bind(1953, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    double layer = c.getVariable(intern("layer"));
    std::vector<bool> chances;
    for (int index = 0; index < 10; index++) chances.push_back(index < layer);
    auto take = js::randomOf(c, chances);
    if (take.has_value() && *take) {
      c.combatStatus(111133 /* StrikeWhereItHurts */);
      double newLayer = std::floor(layer / 2);
      if (newLayer > 0) c.setVariable(intern("layer"), newLayer);
      else c.dispose();
    }
    return JsonValue();
  });
  // clorinde.gts NightVigil on cancelHealed: the healing becomes Bond of Life on the master.
  slots.bind(2370, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto& event = c.e<ModifyHealEventArg>();
    double value = event.expectedValue();
    event.cancel();
    if (value > 0) {
      c.characterStatus(122 /* BondOfLife */, std::vector<AnyHandle>{AnyHandle(&c, c.selfEntity().master().id(), true)},
                        CreateEntityOptions{.overrideVariables = {{intern("usage"), value}}});
    }
    return JsonValue();
  });
  // black_serpent_knight_rockbreaker_ax.gts: +3 against a shield or barrier on the target or the opponent's side.
  slots.bind(2739, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto& event = c.e<ModifyDamageEventArg>();
    auto shielding = [](const EntityDefinition& definition) {
      return definition.hasTag(intern("shield")) || definition.hasTag(intern("barrier"));
    };
    bool underShield = false;
    for (const EntityPointer& entity : event.target()->entities) underShield = underShield || shielding(*entity->definition);
    for (const EntityHandle& status : js::listOfEntities(c.queryAll(query::dollar().opp().combatStatus()))) {
      underShield = underShield || shielding(status.definition());
    }
    event.increaseDamage(underShield ? 3 : 1);
    return JsonValue();
  });
  // kinich.gts: the default snippet on the master (`:get(:e)`), inlined at both call sites; 2212 gains first.
  for (int slot : {2212, 2214, 2215}) {
    slots.bind(slot, [slot](LambdaInvocation& call) -> JsonValue {
      SkillContext& c = *call.context;
      CharacterHandle master = c.selfEntity().master();
      std::vector<AnyHandle> masterTarget{AnyHandle(&c, master.id(), true)};
      if (slot != 2215) c.gainNightsoul(masterTarget);
      auto nightsoul = master.hasNightsoulsBlessing();
      if (nightsoul.has_value() && nightsoul->getVariable(intern("nightsoul")) == 2 && !master.hasStatus(117094 /* GrapplePrepare */).has_value()) {
        master.addStatus(117094 /* GrapplePrepare */);
        c.consumeNightsoul(masterTarget, 2);
      }
      return JsonValue();
    });
  }
  // arlecchino.gts on cancelHealed: only Balemoon Rising heals her.
  slots.bind(3446, [](LambdaInvocation& call) -> JsonValue {
    return JsonValue(call.context->e<ModifyHealEventArg>().via()->definition->id != 13143 /* BalemoonRising */);
  });
  // ally.gts on entityEnter: the entering Simulanka summon gets +1 effect.
  slots.bind(1063, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    c.addVariable(intern("effect"), 1, AnyHandle(&c, c.e<EnterEventArg>().entity.id(), false));
    return JsonValue();
  });
  // arlecchino.gts on cancelHealed (when 3446): cancel the healing.
  slots.bind(3447, [](LambdaInvocation& call) -> JsonValue {
    call.context->e<ModifyHealEventArg>().cancel();
    return JsonValue();
  });
  // jahoda.gts: the flask takes the form of the opponent's active character's aura.
  slots.bind(1453, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    static const std::map<Aura, int> flaskOfAura = {
        {aura::Pyro, 115162}, {aura::Hydro, 115163}, {aura::Electro, 115164}, {aura::Cryo, 115165}, {aura::CryoDendro, 115165}};
    auto active = js::optionalOfCharacters(c.query(query::dollar().opp().active().includesDefeated()));
    auto target = flaskOfAura.find((*active).aura());
    if (target != flaskOfAura.end()) c.transformDefinition(std::vector<AnyHandle>{AnyHandle(&c, c.selfEntity().id(), false)}, target->second);
    return JsonValue();
  });
  // legend.gts: Flames of War on both sides, +1 spirit on mine.
  slots.bind(907, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto myFlame = js::optionalOfEntities(c.query(query::dollar().my().support().def(300006 /* FlamesOfWar */)));
    auto oppFlame = js::optionalOfEntities(c.query(query::dollar().opp().support().def(300006 /* FlamesOfWar */)));
    int who = c.selfEntity().who();
    if (!myFlame) myFlame = c.createEntity(EntityType::Support, 300006, EntityArea{.type = AreaType::Supports, .who = who});
    if (myFlame) c.addVariable(intern("spirit"), 1, AnyHandle(&c, myFlame->id(), false));
    if (!oppFlame) c.createEntity(EntityType::Support, 300006, EntityArea{.type = AreaType::Supports, .who = 1 - who});
    return JsonValue();
  });
  // hydro_tulpa.gts: the master has 3+ health and the summon area has room.
  slots.bind(3210, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    return JsonValue(c.selfEntity().master().health() >= 3 &&
                     static_cast<int>(c.queryAll(query::dollar().my().summon()).size()) < c.state().config.maxSummonsCount);
  });
  // ineffa.gts: a Conductive entering on an opponent's card in hand or pile makes that card untunable.
  slots.bind(2504, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    EntityArea area = c.entityArea(c.e<EnterEventArg>().entity.id());
    if (area.type == AreaType::Hands || area.type == AreaType::Pile) c.attach(207 /* NoTuningAllowed */, EntityHandle(&c, area.cardId));
    return JsonValue();
  });
  // ruin_drake_earthguard.gts (custom): one absorption status per element, in ABSORPTION_EFFECTS order.
  static const std::pair<int, DamageType> absorptionEffects[] = {
      {130051, damage::Cryo}, {130052, damage::Hydro}, {130053, damage::Pyro}, {130054, damage::Electro},
      {130055, damage::Anemo}, {130056, damage::Geo}, {130057, damage::Dendro}};
  auto findAbsorbed = [](const CharacterHandle& character) -> const std::pair<int, DamageType>* {
    for (const auto& effect : absorptionEffects) if (character.hasStatus(effect.first).has_value()) return &effect;
    return nullptr;
  };
  auto absorptionStatusFor = [](DamageType element) -> int {
    for (const auto& effect : absorptionEffects) if (effect.second == element) return effect.first;
    return 0;
  };
  // Close Expulsion: 6 of the absorbed element (dropping the absorption) or 5 physical.
  slots.bind(4988, [findAbsorbed](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    if (const auto* absorbed = findAbsorbed(c.selfCharacter())) {
      c.damage(absorbed->second, 6);
      if (auto status = c.selfCharacter().hasStatus(absorbed->first)) status->dispose();
    } else {
      c.damage(damage::Physical, 5);
    }
    return JsonValue();
  });
  slots.bind(4991, [findAbsorbed, absorptionStatusFor](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    return JsonValue(absorptionStatusFor(c.e<DamageOrHealEventArg>().type()) != 0 && !findAbsorbed(c.selfCharacter()));
  });
  slots.bind(4992, [absorptionStatusFor](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    c.characterStatus(absorptionStatusFor(c.e<DamageOrHealEventArg>().type()), std::vector<AnyHandle>{AnyHandle(&c, c.selfCharacter().id(), true)});
    return JsonValue();
  });
  // other.gts SIMULANKA_QUERY: $.union(...SIMULANKA_SUMMONS.map((id) => $.my.summon.def(id))).
  auto simulankaQuery = [] {
    std::vector<query::Query> operands;
    for (int summon : {301028, 301029, 301030, 301031}) operands.push_back(query::dollar().my().summon().def(summon));
    return query::dollar().union_(operands);
  };
  // place.gts on declareEnd: one of my Simulanka summons is on the field.
  slots.bind(1312, [simulankaQuery](LambdaInvocation& call) -> JsonValue {
    return JsonValue(call.context->query(simulankaQuery()).has_value());
  });
  // place.gts: a random one of them triggers its end phase skill.
  slots.bind(1313, [simulankaQuery](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto chosen = js::randomOf(c, js::listOfEntities(c.queryAll(simulankaQuery())));
    if (chosen) c.triggerEndPhaseSkill(*chosen);
    return JsonValue();
  });
  // other.gts Saurian Dining Buddies: +1 usage on the target's technique if it counts usage.
  slots.bind(750, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto technique = CharacterHandle(&c, c.e<InitiativeSkillArgument>().targets.at(0).id()).hasTechnique();
    if (technique && technique->definition().findVarConfig(intern("usage"))) {
      c.addVariable(intern("usage"), 1, AnyHandle(&c, technique->id(), false));
    }
    return JsonValue();
  });
  // techniques.gts Waverider on staged: a shield on the target.
  slots.bind(236, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    c.characterStatus(301304 /* WaveriderShield */, std::vector<AnyHandle>{AnyHandle(&c, c.e<InitiativeSkillArgument>().targets.at(0).id(), true)});
    return JsonValue();
  });
  // alldevouring_narwhal.gts LightlessFeeding: heal by the discarded card's cost (the event carries its id).
  slots.bind(3014, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    int cardId = static_cast<int>(c.e<CustomEventEventArg>().arg.asNumber());
    c.heal(EntityHandle(&c, cardId).diceCost(), std::vector<AnyHandle>{AnyHandle(&c, c.selfEntity().master().id(), true)});
    return JsonValue();
  });
  // goldflame_qucusaur_tyrant.gts GoldflameState end phase: with Flamelord's Blessing, discard a random hand card
  // and add its cost to the damage.
  slots.bind(3598, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    std::optional<EntityHandle> chosen;
    if (c.query(query::dollar().my().equipped().def(223061 /* FlamelordsBlessing */)).has_value()) {
      chosen = js::randomOf(c, js::entityHandles(c, c.player().hands));
    }
    double damageValue = 1 + (chosen ? chosen->diceCost() : 0);
    c.damage(damage::Piercing, 1, c.queryAll(query::dollar().opp().standby()));
    c.damage(damage::Pyro, damageValue);
    if (chosen) c.discard(std::vector<EntityHandle>{*chosen});
    return JsonValue();
  });
  // techniques.gts Blubberbeast Sonar Waves: draw a random pile card of the highest or lowest cost.
  slots.bind(250, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    c.abortPreview();
    c.switchActive(c.queryAll(query::dollar().my().next()));
    bool takeMax = *js::randomOf(c, std::vector<bool>{true, false});
    std::map<int, std::vector<EntityHandle>> pileByCost;  // Object.groupBy: integer keys ascending, pile order inside
    for (const EntityHandle& card : js::entityHandles(c, c.player().pile)) pileByCost[card.diceCost()].push_back(card);
    if (pileByCost.empty()) return JsonValue();
    const auto& candidates = takeMax ? pileByCost.rbegin()->second : pileByCost.begin()->second;
    auto targetCard = js::randomOf(c, candidates);
    if (targetCard) c.drawCards(std::vector<EntityHandle>{*targetCard});
    return JsonValue();
  });
  // food.gts: four random picks among heal 2, +1 max health, Battle Plan, Sharpen the Blade on the target.
  slots.bind(489, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    c.abortPreview();
    std::vector<AnyHandle> target{AnyHandle(&c, c.e<InitiativeSkillArgument>().targets.at(0).id(), true)};
    for (int round = 0; round < 4; round++) {
      switch (static_cast<int>(*js::randomOf(c, std::vector<double>{0, 1, 2, 3}))) {
        case 0: c.heal(2, target); break;
        case 1: c.increaseMaxHealth(1, target); break;
        case 2: c.characterStatus(172 /* BattlePlan */, target); break;
        default: c.characterStatus(209 /* SharpenTheBlade */, target); break;
      }
    }
    return JsonValue();
  });
  // kaveh.gts: disposed by this entity's own skill.
  slots.bind(2193, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    const auto& via = c.e<DisposeEventArg>().via;
    return JsonValue(via != nullptr && via->caller.id() == c.skillInfo().caller.id());
  });
  // xilonen.gts burst: each Source Sample layer adds a draw (Geo) or a heal (others). Object.entries order.
  slots.bind(2930, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    c.damage(damage::Geo, 2);
    double healCount = 1, drawCount = 1;
    for (int sample : {116116, 116114, 116115, 116117, 116113}) {
      auto status = js::optionalOfEntities(c.query(query::dollar().my().typeStatus().def(sample)));
      if (!status) continue;
      if (sample == 116113 /* SourceSampleGeo */) drawCount += status->getVariable(intern("layer"));
      else healCount += status->getVariable(intern("layer"));
    }
    c.heal(healCount, c.queryAll(query::dollar().macros().myMostInjured()));
    c.drawCards(static_cast<int>(drawCount));
    return JsonValue();
  });
  // other.gts: Melusine's event cards that exist in this version's data.
  auto melusineEventCards = [](SkillContext& c) {
    std::vector<double> cards;
    for (int id : {331102, 331202, 331302, 331402, 331502, 331602, 331702, 331801, 331802, 331803, 331804, 331805, 331806, 331807}) {
      if (c.state().data->entities.find(id)) cards.push_back(id);
    }
    return cards;
  };
  slots.bind(733, [melusineEventCards](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto cards = melusineEventCards(c);
    auto card0 = js::randomOf(c, cards);
    auto card1 = js::randomOf(c, cards);
    c.createHandCard(*card0);
    c.createHandCard(*card1);
    return JsonValue();
  });
  slots.bind(734, [melusineEventCards](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto cards = melusineEventCards(c);
    c.createHandCard(*js::randomOf(c, cards));
    return JsonValue();
  });
  // other.gts: move the first target's weapon (usage per round reset) to the second target, replacing its weapon.
  slots.bind(634, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    const auto& targets = c.e<InitiativeSkillArgument>().targets;
    EntityHandle weapon = *CharacterHandle(&c, targets.at(0).id()).hasWeapon();
    weapon.resetUsagePerRound();
    CharacterHandle target(&c, targets.at(1).id());
    EntityArea area{.type = AreaType::Characters, .who = target.who(), .characterId = target.id()};
    if (auto oldWeapon = target.hasWeapon()) c.dispose(std::vector<AnyHandle>{AnyHandle(&c, oldWeapon->id(), false)});
    c.moveEntity(weapon, area);
    return JsonValue();
  });
  // lord_of_eroded_primal_fire.gts: discard the top 3 pile cards of both players (in reverse), then Resentment.
  slots.bind(3643, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto resentment = c.selfCharacter().hasStatus(123051 /* Resentment */);
    double layer = resentment ? resentment->getVariable(intern("layer")) : 0;
    c.damage(damage::Piercing, layer + 1, c.queryAll(query::dollar().opp().standby()));
    c.damage(damage::Pyro, 1);
    c.abortPreview();
    for (bool opponent : {false, true}) {
      auto pile = js::slice(js::entityHandles(c, opponent ? c.oppPlayer().pile : c.player().pile), 0, 3);
      for (auto card = pile.rbegin(); card != pile.rend(); ++card) c.discard(std::vector<EntityHandle>{*card});
    }
    c.characterStatus(123051 /* Resentment */, std::vector<AnyHandle>{AnyHandle(&c, c.selfCharacter().id(), true)});
    return JsonValue();
  });
  // place.gts: at least 5 different dice counting each omni die as its own kind (`new Set()` of dice).
  slots.bind(1239, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    int omniCount = 0;
    std::vector<double> nonOmniDice;
    for (double die : js::numbers(c.player().dice)) {
      if (die == dice::Omni) omniCount++;
      else if (std::find(nonOmniDice.begin(), nonOmniDice.end(), die) == nonOmniDice.end()) nonOmniDice.push_back(die);
    }
    return JsonValue(omniCount + static_cast<int>(nonOmniDice.size()) >= 5);
  });
  // gluttonous_yumkasaur_mountain_king.gts: random pick of WellFedAndStrong, WellFedAndSturdy or 1 max health.
  slots.bind(2143, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    c.abortPreview();
    constexpr double increaseMaxHealth = -1;
    double choice = *js::randomOf(c, std::vector<double>{127041 /* WellFedAndStrong */, 127042 /* WellFedAndSturdy */, increaseMaxHealth});
    std::vector<AnyHandle> self{AnyHandle(&c, c.selfCharacter().id(), true)};
    if (choice == increaseMaxHealth) c.increaseMaxHealth(1.0, self);
    else c.characterStatus(choice, self);
    return JsonValue();
  });
  // xilonen.gts on actionPhase: with 2+ nightsoul, each Source Sample on our side gives the opponent its DMG bonus.
  // Object.entries visits integer keys in ascending order: Cryo, Hydro, Pyro, Electro, Geo.
  slots.bind(2937, [](LambdaInvocation& call) -> JsonValue {
    SkillContext& c = *call.context;
    auto nightsoul = c.selfCharacter().hasStatus(116111 /* NightsoulsBlessing */);
    if (nightsoul.has_value() && (*nightsoul).getVariable(intern("nightsoul")) >= 2.0) {
      static const std::pair<int, int> sampleAndBonus[] = {
          {116116, 216116} /* Cryo */, {116114, 216114} /* Hydro */, {116115, 216115} /* Pyro */,
          {116117, 216117} /* Electro */, {116113, 216113} /* Geo */};
      for (const auto& [sample, bonus] : sampleAndBonus) {
        if (c.query(query::dollar().my().typeStatus().def(sample)).has_value()) c.combatStatus(bonus, true);
      }
      c.consumeNightsoul(std::vector<AnyHandle>{AnyHandle(&c, c.selfCharacter().id(), true)}, 2.0);
    }
    return JsonValue();
  });
  // gluttonous_yumkasaur_mountain_king.gts: the entity's default snippet (called with no argument).
  slots.bind(2145, [](LambdaInvocation& call) -> JsonValue {
    call.context->callSnippet("default", nullptr);
    return JsonValue();
  });
  bindFinishHandPorts(slots);
  bindSweepHandPorts(slots);
}

}  // namespace gitcg
