// Numeric game enums with the same values as @gi-tcg/typings (the JSON protocol carries these numbers).
// DiceType and DiceRequirementType share one number space: Omni and Aligned are both 8.
#pragma once

namespace gitcg {

using DiceType = int;
namespace dice {
inline constexpr DiceType Void = 0;
inline constexpr DiceType Cryo = 1;
inline constexpr DiceType Hydro = 2;
inline constexpr DiceType Pyro = 3;
inline constexpr DiceType Electro = 4;
inline constexpr DiceType Anemo = 5;
inline constexpr DiceType Geo = 6;
inline constexpr DiceType Dendro = 7;
inline constexpr DiceType Omni = 8;
inline constexpr DiceType Aligned = 8;
inline constexpr DiceType Energy = 9;
inline constexpr DiceType Legend = 10;
}  // namespace dice

using DamageType = int;
namespace damage {
inline constexpr DamageType Physical = 0;
inline constexpr DamageType Cryo = 1;
inline constexpr DamageType Hydro = 2;
inline constexpr DamageType Pyro = 3;
inline constexpr DamageType Electro = 4;
inline constexpr DamageType Anemo = 5;
inline constexpr DamageType Geo = 6;
inline constexpr DamageType Dendro = 7;
inline constexpr DamageType Piercing = 8;
inline constexpr DamageType Heal = 9;
}  // namespace damage

using Aura = int;
namespace aura {
inline constexpr Aura None = 0;
inline constexpr Aura Cryo = 1;
inline constexpr Aura Hydro = 2;
inline constexpr Aura Pyro = 3;
inline constexpr Aura Electro = 4;
inline constexpr Aura Dendro = 7;
inline constexpr Aura CryoDendro = 113;
}  // namespace aura

// 0 stands for "no reaction" (JS null), as in the protocol's UNSPECIFIED.
using Reaction = int;
namespace reaction {
inline constexpr Reaction None = 0;
inline constexpr Reaction Melt = 101;
inline constexpr Reaction Vaporize = 102;
inline constexpr Reaction Overloaded = 103;
inline constexpr Reaction Superconduct = 104;
inline constexpr Reaction ElectroCharged = 105;
inline constexpr Reaction Frozen = 106;
inline constexpr Reaction SwirlCryo = 107;
inline constexpr Reaction SwirlHydro = 108;
inline constexpr Reaction SwirlPyro = 109;
inline constexpr Reaction SwirlElectro = 110;
inline constexpr Reaction CrystallizeCryo = 111;
inline constexpr Reaction CrystallizeHydro = 112;
inline constexpr Reaction CrystallizePyro = 113;
inline constexpr Reaction CrystallizeElectro = 114;
inline constexpr Reaction Burning = 115;
inline constexpr Reaction Bloom = 116;
inline constexpr Reaction Quicken = 117;
inline constexpr Reaction LunarElectroCharged = 3001;
inline constexpr Reaction LunarBloom = 3002;
inline constexpr Reaction LunarCrystallizeHydro = 3003;
}  // namespace reaction

enum class Phase { InitHands, InitActives, Roll, Action, End, GameEnd };

namespace action_validity {
inline constexpr int Valid = 0;
inline constexpr int ConditionNotMet = 1;
inline constexpr int NoTarget = 2;
inline constexpr int NoDice = 3;
inline constexpr int NoEnergy = 4;
inline constexpr int Disabled = 5;
inline constexpr int NoLegend = 6;
}  // namespace action_validity

}  // namespace gitcg
