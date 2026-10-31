// Rockfond Rifthound (3602): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages "Corrosive Claws": 1 Geo, "Crystal Cracker": 2 Geo,
// "Pursuer's Mark": 6 Geo. Corrosion (136021) is shared with the Thundercraven Rifthound (corrosion.gts).
import { DamageType, DiceType } from "@gi-tcg/core/data";
import { Corrosion } from "./corrosion.gts";

/**
 * @id 136022
 * @name Elemental Devourer
 * @description
 * After taking Geo DMG, stack 1 level of Elemental Devourer. (Max 2 levels)
 * When 2 levels of Elemental Devourer have been gained, character deals +1 Geo DMG and takes +2 Geo DMG.
 */
define status {
  id 136022 as GeoElementalDevourer;
  variable level, 0;
  on damaged {
    when :( :e.type === DamageType.Geo && :getVariable("level") < 2 );
    :addVariable("level", 1);
  };
  on increaseSkillDamage {
    when :( :getVariable("level") >= 2 && :e.type === DamageType.Geo );
    :e.increaseDamage(1);
  };
  on increaseDamaged {
    when :( :getVariable("level") >= 2 && :e.type === DamageType.Geo );
    :e.increaseDamage(2);
  };
};

/**
 * @id 36021
 * @name Corrosive Claws
 * @description
 * Deals 1 Geo DMG, creates 1 Corrosion at the opponent's play area.
 */
define skill {
  id 36021 as CorrosiveClaws;
  skillType normal;
  cost DiceType.Geo, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Geo, 1);
  :combatStatus(Corrosion, "opp");
};

/**
 * @id 36022
 * @name Crystal Cracker
 * @description
 * Deals 2 Geo DMG, creates 1 Corrosion at the opponent's play area.
 */
define skill {
  id 36022 as CrystalCracker;
  skillType elemental;
  cost DiceType.Geo, 3;
  :damage(DamageType.Geo, 2);
  :combatStatus(Corrosion, "opp");
};

/**
 * @id 36023
 * @name Pursuer's Mark
 * @description
 * Deals 6 Geo DMG, creates 1 Corrosion at the opponent's play area.
 */
define skill {
  id 36023 as PursuersMark;
  skillType burst;
  cost DiceType.Geo, 3;
  cost DiceType.Energy, 3;
  :damage(DamageType.Geo, 6);
  :combatStatus(Corrosion, "opp");
};

/**
 * @id 36024
 * @name Elemental Devourer
 * @description
 * (Passive) When the battle begins, this character gains Elemental Devourer.
 */
define skill {
  id 36024 as GeoElementalDevourerPassive;
  skillType passive {
    on battleBegin {
      :characterStatus(GeoElementalDevourer, :self);
    };
  };
};

/**
 * @id 3602
 * @name Rockfond Rifthound
 * @hp 10
 * @energy 3
 */
define character {
  id 3602 as RockfondRifthound;
  tags geo, monster;
  health 10;
  energy 3;
  skills CorrosiveClaws, CrystalCracker, PursuersMark, GeoElementalDevourerPassive;
};
