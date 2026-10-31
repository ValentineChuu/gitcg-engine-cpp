// Hydro Samachurl (3202): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages ("Staff Strike (Hydro Samachurl)": 1 Physical,
// Prayer for Rain: 1 Hydro, Tidal Impact: 5 Hydro).
import { $, DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 132021
 * @name Healing Rain
 * @description
 * End Phase: Heal your active character for 1 HP and grant both sides' active characters Hydro Application.
 * Usage(s): 2
 */
define summon {
  id 132021 as HealingRain;
  hint DamageType.Heal, 1;
  on endPhase {
    usage 2;
    :heal(1, $.my.active);
    :apply(DamageType.Hydro, $.my.active);
    :apply(DamageType.Hydro, $.opp.active);
  };
};

/**
 * @id 32021
 * @name Staff Strike
 * @description
 * Deals 1 Physical DMG.
 */
define skill {
  id 32021 as HydroSamachurlStaffStrike;
  skillType normal;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 1);
};

/**
 * @id 32022
 * @name Prayer for Rain
 * @description
 * Deals 1 Hydro DMG, summons 1 Healing Rain.
 */
define skill {
  id 32022 as PrayerForRain;
  skillType elemental;
  cost DiceType.Hydro, 3;
  :damage(DamageType.Hydro, 1);
  :summon(HealingRain);
};

/**
 * @id 32023
 * @name Tidal Impact
 * @description
 * Deals 5 Hydro DMG.
 */
define skill {
  id 32023 as TidalImpact;
  skillType burst;
  cost DiceType.Hydro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Hydro, 5);
};

/**
 * @id 3202
 * @name Hydro Samachurl
 * @hp 4
 * @energy 2
 */
define character {
  id 3202 as HydroSamachurl;
  tags hydro, monster, hilichurl;
  health 4;
  energy 2;
  skills HydroSamachurlStaffStrike, PrayerForRain, TidalImpact;
};
