// Anemo Samachurl (3501): NPC-only monster written from the game description.
// Damage values: Dimbreath declared values (Prayer to the Winds: 2 Anemo) and fandom wiki skill pages
// ("Staff Strike (Anemo Samachurl)": 1 Physical, Galeforce Shock: 5 Anemo, Whirlwind: 1 Anemo x2).
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 135011
 * @name Whirlwind
 * @description
 * End Phase: Deal 1 Anemo DMG.
 * Usage(s): 2
 */
define summon {
  id 135011 as Whirlwind;
  hint DamageType.Anemo, 1;
  on endPhase {
    usage 2;
    :damage(DamageType.Anemo, 1);
  };
};

/**
 * @id 35011
 * @name Staff Strike
 * @description
 * Deals 1 Physical DMG.
 */
define skill {
  id 35011 as AnemoSamachurlStaffStrike;
  skillType normal;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 1);
};

/**
 * @id 35012
 * @name Prayer to the Winds
 * @description
 * Deals 2 Anemo DMG, summons 1 Whirlwind.
 */
define skill {
  id 35012 as PrayerToTheWinds;
  skillType elemental;
  cost DiceType.Anemo, 3;
  :damage(DamageType.Anemo, 2);
  :summon(Whirlwind);
};

/**
 * @id 35013
 * @name Galeforce Shock
 * @description
 * Deals 5 Anemo DMG.
 */
define skill {
  id 35013 as GaleforceShock;
  skillType burst;
  cost DiceType.Anemo, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Anemo, 5);
};

/**
 * @id 3501
 * @name Anemo Samachurl
 * @hp 4
 * @energy 2
 */
define character {
  id 3501 as AnemoSamachurl;
  tags anemo, monster, hilichurl;
  health 4;
  energy 2;
  skills AnemoSamachurlStaffStrike, PrayerToTheWinds, GaleforceShock;
};
