// Fatui Skirmisher - Geochanter Bracer (3604): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages "Rock Projectile": 1 Geo, "Geocrystal Beam": 3 Geo,
// "Staff of Geo": 3 Geo (the wiki calls the combat status "Crystal Shield"; the game data says Crystal Ward).
// Geo Elemental Equipment follows weekly_b's Craggy Greatshield (Rock Shieldwall Mitachurl), with 1 DMG off.
import { $, DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 136042
 * @name Crystal Ward
 * @description
 * When your active character takes DMG: Decrease DMG taken by 1.
 * Usage(s): 3
 * When Geo Elemental Equipment leaves play: Remove this effect.
 */
define combatStatus {
  id 136042 as CrystalWard;
  on decreaseDamaged {
    when :( :e.value > 0 && :e.target.isActive() );
    usage 3;
    :e.decreaseDamage(1);
  };
};

/**
 * @id 136041
 * @name Geo Elemental Equipment
 * @description
 * When the character to which this is attached takes DMG: Decrease 1 DMG.
 * When Geo DMG is decreased, consume 1 additional Usage(s).
 * Usage(s): 3
 */
define status {
  id 136041 as GeoElementalEquipment;
  tags barrier;
  on decreaseDamaged {
    when :( :e.value > 0 );
    usage 3;
    :e.decreaseDamage(1);
  };
  on damaged {
    when :( :e.type === DamageType.Geo );
    :consumeUsage(1);
  };
  on selfDispose {
    :query($.my.combatStatus.def(CrystalWard))?.dispose();
  };
};

/**
 * @id 36041
 * @name Rock Projectile
 * @description
 * Deals 1 Geo DMG.
 */
define skill {
  id 36041 as RockProjectile;
  skillType normal;
  cost DiceType.Geo, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Geo, 1);
};

/**
 * @id 36042
 * @name Geocrystal Beam
 * @description
 * Deals 3 Geo DMG.
 */
define skill {
  id 36042 as GeocrystalBeam;
  skillType elemental;
  cost DiceType.Geo, 3;
  :damage(DamageType.Geo, 3);
};

/**
 * @id 36043
 * @name Staff of Geo
 * @description
 * Deals 3 Geo DMG. This character gains Geo Elemental Equipment, creating 1 Crystal Ward.
 */
define skill {
  id 36043 as StaffOfGeo;
  skillType burst;
  cost DiceType.Geo, 3;
  cost DiceType.Energy, 3;
  :damage(DamageType.Geo, 3);
  :characterStatus(GeoElementalEquipment, :self);
  :combatStatus(CrystalWard);
};

/**
 * @id 36044
 * @name Combat Resupply
 * @description
 * (Passive) When the battle begins, this character gains 2 Energy.
 */
define skill {
  id 36044 as GeochanterCombatResupply;
  skillType passive {
    on battleBegin {
      :gainEnergy(2, :self);
    };
  };
};

/**
 * @id 3604
 * @name Fatui Skirmisher - Geochanter Bracer
 * @hp 10
 * @energy 3
 */
define character {
  id 3604 as FatuiSkirmisherGeochanterBracer;
  tags geo, fatui;
  health 10;
  energy 3;
  skills RockProjectile, GeocrystalBeam, StaffOfGeo, GeochanterCombatResupply;
};
