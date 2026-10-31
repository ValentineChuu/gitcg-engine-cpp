// Hand ports of the extensions' `mutateWhen` hooks: `(state, event) => ...` mutating the extension's JSON state.
// Each binding names its slot, source file and the JS it replaces. (They don't take a skill context, so the
// translator doesn't handle them.)

#include <algorithm>

#include "cards/translated/translated_card_support.h"

namespace gitcg {

namespace {

JsonValue& pairEntry(JsonValue& state, const char* field, int who) {
  return state[field].asArray().at(static_cast<size_t>(who));
}

void increment(JsonValue& state, const char* field, int who) {
  JsonValue& value = pairEntry(state, field, who);
  value = JsonValue(value.asNumber() + 1);
}

// `list[who]` gets `value` unless it already holds it.
void addUnique(JsonValue& state, const char* field, int who, double value) {
  JsonValue& list = pairEntry(state, field, who);
  for (const JsonValue& item : list.asArray()) {
    if (item.asNumber() == value) return;
  }
  list.push(JsonValue(value));
}

template <typename Argument>
Argument& eventOf(LambdaInvocation& call) {
  return static_cast<Argument&>(*call.argument);
}

}  // namespace

void bindExtensionHooks(LambdaSlots& slots) {
  static const Name TECHNIQUE = intern("technique");
  // techniques.gts:338 onPlayCard: if (e.card.definition.tags.includes("technique")) c.techniquesPlayedCount[e.who]++
  slots.bind(240, [](LambdaInvocation& call) -> JsonValue {
    auto& e = eventOf<PlayCardEventArg>(call);
    if (e.card()->definition->hasTag(TECHNIQUE)) increment(*call.extensionState, "techniquesPlayedCount", e.who);
    return JsonValue();
  });
  // claymore.gts:248 onPlayCard: cards not in the initial pile, once per definition id.
  slots.bind(343, [](LambdaInvocation& call) -> JsonValue {
    auto& e = eventOf<PlayCardEventArg>(call);
    int definitionId = e.card()->definition->id;
    const auto& initialPile = e.onTimeState->players[e.who]->initialPile;
    bool inInitialPile = std::any_of(initialPile.begin(), initialPile.end(),
                                     [&](const EntityDefinition* card) { return card->id == definitionId; });
    if (!inInitialPile) addUnique(*call.extensionState, "defIds", e.who, definitionId);
    return JsonValue();
  });
  // other.gts:1316 onRoundEnd: c.first = [true, true]
  slots.bind(676, [](LambdaInvocation& call) -> JsonValue {
    (*call.extensionState)["first"] = JsonValue(JsonValue::Array{JsonValue(true), JsonValue(true)});
    return JsonValue();
  });
  // other.gts:1317 onPlayCard: c.first[e.who] = false
  slots.bind(677, [](LambdaInvocation& call) -> JsonValue {
    pairEntry(*call.extensionState, "first", eventOf<PlayCardEventArg>(call).who) = JsonValue(false);
    return JsonValue();
  });
  // other.gts:2635 onDispose: disposed supports and summons, not discards or tunings.
  slots.bind(794, [](LambdaInvocation& call) -> JsonValue {
    auto& e = eventOf<DisposeEventArg>(call);
    if (e.isDiscardOrTuning()) return JsonValue();
    EntityType type = e.disposedEntity()->definition->type;
    if (type == EntityType::Support) increment(*call.extensionState, "disposedSupportCount", e.who);
    else if (type == EntityType::Summon) increment(*call.extensionState, "disposedSummonsCount", e.who);
    return JsonValue();
  });
  // ally.gts:613 onDispose: disposed supports, not discards or tunings.
  slots.bind(1031, [](LambdaInvocation& call) -> JsonValue {
    auto& e = eventOf<DisposeEventArg>(call);
    if (!e.isDiscardOrTuning() && e.disposedEntity()->definition->type == EntityType::Support) {
      increment(*call.extensionState, "disposedSupportCount", e.who);
    }
    return JsonValue();
  });
  // ally.gts:676 onDamageOrHeal: elemental damage types each side has taken.
  slots.bind(1039, [](LambdaInvocation& call) -> JsonValue {
    auto& e = eventOf<DamageOrHealEventArg>(call);
    if (e.isDamageTypeDamage() && e.type() != damage::Physical && e.type() != damage::Piercing) {
      addUnique(*call.extensionState, "damages", e.targetWho, e.type());
    }
    return JsonValue();
  });
  // item.gts:174 onAction: action cards each side has played.
  slots.bind(1210, [](LambdaInvocation& call) -> JsonValue {
    auto& e = eventOf<ActionEventArg>(call);
    if (e.isPlayCard()) addUnique(*call.extensionState, "played", e.who, e.action().skill->caller.entity()->definition->id);
    return JsonValue();
  });
  // ganyu.gts:87 onDamageOrHeal: st.used[e.targetWho] = false when Ganyu (1101) is defeated.
  slots.bind(1873, [](LambdaInvocation& call) -> JsonValue {
    auto& e = eventOf<DamageOrHealEventArg>(call);
    if (e.target()->definition->id == 1101 && e.damageInfo().causeDefeated) pairEntry(*call.extensionState, "used", e.targetWho) = JsonValue(false);
    return JsonValue();
  });
  // qiqi.gts:112 onDamageOrHeal: st.count[e.targetWho] = 0 when Qiqi (1108) is defeated.
  slots.bind(1947, [](LambdaInvocation& call) -> JsonValue {
    auto& e = eventOf<DamageOrHealEventArg>(call);
    if (e.target()->definition->id == 1108 && e.damageInfo().causeDefeated) pairEntry(*call.extensionState, "count", e.targetWho) = JsonValue(0);
    return JsonValue();
  });
  // wayward_hermetic_spiritspeaker.gts:302 onUseSkill: st.usedCount[e.who]++ for Radiance in Flux.
  slots.bind(2034, [](LambdaInvocation& call) -> JsonValue {
    auto& e = eventOf<UseSkillEventArg>(call);
    if (e.skill->definition->id == 21052 /* RadianceInFlux */) increment(*call.extensionState, "usedCount", e.who);
    return JsonValue();
  });
  // columbina.gts:234 onReaction: lunar reactions per side.
  slots.bind(3065, [](LambdaInvocation& call) -> JsonValue {
    auto& e = eventOf<ReactionEventArg>(call);
    Reaction type = e.type();
    if (type == reaction::LunarElectroCharged || type == reaction::LunarBloom || type == reaction::LunarCrystallizeHydro) {
      increment(*call.extensionState, "reactionCount", e.who);
    }
    return JsonValue();
  });
}

}  // namespace gitcg
