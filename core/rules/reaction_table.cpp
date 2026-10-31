#include "core/rules/reaction_table.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace gitcg {

namespace {

// Rows: aura; columns: Cryo, Hydro, Pyro, Electro, Anemo, Geo, Dendro.
struct AuraRow {
  Aura aura;
  std::pair<Aura, Reaction> byElement[7];
};

constexpr AuraRow REACTION_MAP[] = {
    {aura::None,
     {{aura::Cryo, reaction::None}, {aura::Hydro, reaction::None}, {aura::Pyro, reaction::None},
      {aura::Electro, reaction::None}, {aura::None, reaction::None}, {aura::None, reaction::None},
      {aura::Dendro, reaction::None}}},
    {aura::Cryo,
     {{aura::Cryo, reaction::None}, {aura::None, reaction::Frozen}, {aura::None, reaction::Melt},
      {aura::None, reaction::Superconduct}, {aura::None, reaction::SwirlCryo}, {aura::None, reaction::CrystallizeCryo},
      {aura::CryoDendro, reaction::None}}},
    {aura::Hydro,
     {{aura::None, reaction::Frozen}, {aura::Hydro, reaction::None}, {aura::None, reaction::Vaporize},
      {aura::None, reaction::ElectroCharged}, {aura::None, reaction::SwirlHydro}, {aura::None, reaction::CrystallizeHydro},
      {aura::None, reaction::Bloom}}},
    {aura::Pyro,
     {{aura::None, reaction::Melt}, {aura::None, reaction::Vaporize}, {aura::Pyro, reaction::None},
      {aura::None, reaction::Overloaded}, {aura::None, reaction::SwirlPyro}, {aura::None, reaction::CrystallizePyro},
      {aura::None, reaction::Burning}}},
    {aura::Electro,
     {{aura::None, reaction::Superconduct}, {aura::None, reaction::ElectroCharged}, {aura::None, reaction::Overloaded},
      {aura::Electro, reaction::None}, {aura::None, reaction::SwirlElectro}, {aura::None, reaction::CrystallizeElectro},
      {aura::None, reaction::Quicken}}},
    {aura::Dendro,
     {{aura::CryoDendro, reaction::None}, {aura::None, reaction::Bloom}, {aura::None, reaction::Burning},
      {aura::None, reaction::Quicken}, {aura::Dendro, reaction::None}, {aura::Dendro, reaction::None},
      {aura::Dendro, reaction::None}}},
    {aura::CryoDendro,
     {{aura::CryoDendro, reaction::None}, {aura::Dendro, reaction::Frozen}, {aura::Dendro, reaction::Melt},
      {aura::Dendro, reaction::Superconduct}, {aura::Dendro, reaction::SwirlCryo}, {aura::Dendro, reaction::CrystallizeCryo},
      {aura::CryoDendro, reaction::None}}},
};

struct ReactionRelatives {
  Reaction reaction;
  DamageType first;
  DamageType second;
};

constexpr ReactionRelatives REACTION_RELATIVES[] = {
    {reaction::Melt, damage::Pyro, damage::Cryo},
    {reaction::Vaporize, damage::Pyro, damage::Hydro},
    {reaction::Overloaded, damage::Pyro, damage::Electro},
    {reaction::Superconduct, damage::Cryo, damage::Electro},
    {reaction::ElectroCharged, damage::Hydro, damage::Electro},
    {reaction::Frozen, damage::Cryo, damage::Hydro},
    {reaction::SwirlCryo, damage::Cryo, damage::Anemo},
    {reaction::SwirlHydro, damage::Hydro, damage::Anemo},
    {reaction::SwirlPyro, damage::Pyro, damage::Anemo},
    {reaction::SwirlElectro, damage::Electro, damage::Anemo},
    {reaction::CrystallizeCryo, damage::Cryo, damage::Geo},
    {reaction::CrystallizeHydro, damage::Hydro, damage::Geo},
    {reaction::CrystallizePyro, damage::Pyro, damage::Geo},
    {reaction::CrystallizeElectro, damage::Electro, damage::Geo},
    {reaction::Burning, damage::Pyro, damage::Dendro},
    {reaction::Bloom, damage::Dendro, damage::Hydro},
    {reaction::Quicken, damage::Dendro, damage::Electro},
    {reaction::LunarElectroCharged, damage::Hydro, damage::Electro},
    {reaction::LunarBloom, damage::Dendro, damage::Hydro},
    {reaction::LunarCrystallizeHydro, damage::Hydro, damage::Geo},
};

Reaction lunarForm(Reaction plain) {
  switch (plain) {
    case reaction::ElectroCharged: return reaction::LunarElectroCharged;
    case reaction::Bloom: return reaction::LunarBloom;
    case reaction::CrystallizeHydro: return reaction::LunarCrystallizeHydro;
    default: return plain;
  }
}

}  // namespace

ReactionOutcome getReaction(DamageType type, Aura targetAura, const std::vector<Reaction>& enabledLunarReactions) {
  if (type == damage::Physical || type == damage::Piercing) {
    return {targetAura, reaction::None};
  }
  for (const AuraRow& row : REACTION_MAP) {
    if (row.aura != targetAura) continue;
    auto [newAura, plain] = row.byElement[type - damage::Cryo];
    if (plain == reaction::None) return {newAura, reaction::None};
    Reaction lunar = lunarForm(plain);
    bool enabled = std::find(enabledLunarReactions.begin(), enabledLunarReactions.end(), lunar) != enabledLunarReactions.end();
    return {newAura, enabled ? lunar : plain};
  }
  throw std::logic_error("unknown aura");
}

bool reactionInvolves(Reaction reactionType, DamageType element) {
  for (const ReactionRelatives& relatives : REACTION_RELATIVES) {
    if (relatives.reaction == reactionType) return relatives.first == element || relatives.second == element;
  }
  throw std::logic_error("unknown reaction");
}

DamageType swirledElement(DamageType type, Aura targetAura) {
  switch (getReaction(type, targetAura, {}).reaction) {
    case reaction::SwirlCryo: return damage::Cryo;
    case reaction::SwirlElectro: return damage::Electro;
    case reaction::SwirlHydro: return damage::Hydro;
    case reaction::SwirlPyro: return damage::Pyro;
    default: return -1;
  }
}

}  // namespace gitcg
