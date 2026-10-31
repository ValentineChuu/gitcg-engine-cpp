// Geo Samachurl (3603): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages "Staff Strike (Geo Samachurl)": 1 Physical,
// "Summoning of Stones": 2 Geo, "Shockwave Smash": 5 Geo.
// Rocky Pedestal is a combat status (the data marks it "onstage", not a summon).
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 136031
 * @name Rocky Pedestal
 * @description
 * When your active Hilichurl takes DMG: Decreases DMG taken by 1.
 * Usage(s): 2
 */
define combatStatus {
  id 136031 as RockyPedestal;
  on decreaseDamaged {
    when :(
      :e.value > 0 &&
        :e.target.isActive() &&
        :e.target.definition.tags.includes("hilichurl")
    );
    usage 2;
    :e.decreaseDamage(1);
  };
};

/**
 * @id 36031
 * @name Staff Strike
 * @description
 * Deals 1 Physical DMG.
 */
define skill {
  id 36031 as GeoSamachurlStaffStrike;
  skillType normal;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 1);
};

/**
 * @id 36032
 * @name Summoning of Stones
 * @description
 * Deals 2 Geo DMG, creates 1 Rocky Pedestal.
 */
define skill {
  id 36032 as SummoningOfStones;
  skillType elemental;
  cost DiceType.Geo, 3;
  :damage(DamageType.Geo, 2);
  :combatStatus(RockyPedestal);
};

/**
 * @id 36033
 * @name Shockwave Smash
 * @description
 * Deals 5 Geo DMG.
 */
define skill {
  id 36033 as ShockwaveSmash;
  skillType burst;
  cost DiceType.Geo, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Geo, 5);
};

/**
 * @id 3603
 * @name Geo Samachurl
 * @hp 4
 * @energy 2
 */
define character {
  id 3603 as GeoSamachurl;
  tags geo, monster, hilichurl;
  health 4;
  energy 2;
  skills GeoSamachurlStaffStrike, SummoningOfStones, ShockwaveSmash;
};
