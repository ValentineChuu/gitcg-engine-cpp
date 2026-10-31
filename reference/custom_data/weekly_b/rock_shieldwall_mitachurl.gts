// Rock Shieldwall Mitachurl (3601): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages "Plata Plama/Da Plama/Aba Movo (Rock Shieldwall Mitachurl)":
// 2 / 3 / 5 Physical. Craggy Greatshield follows Piovium's Stonehide (126011) pattern.
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 136011
 * @name Craggy Greatshield
 * @description
 * When the character to which this is attached takes DMG: Decrease a maximum of 2 DMG.
 * When Geo DMG is decreased, consume 1 additional Usage(s).
 * Usage(s): 3
 */
define status {
  id 136011 as CraggyGreatshield;
  tags barrier;
  on decreaseDamaged {
    usage 3;
    :e.decreaseDamage(Math.min(2, :e.value));
  };
  on damaged {
    when :( :e.type === DamageType.Geo );
    :consumeUsage(1);
  };
};

/**
 * @id 36011
 * @name Plata Plama
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 36011 as RockShieldwallPlataPlama;
  skillType normal;
  cost DiceType.Geo, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 36012
 * @name Da Plama
 * @description
 * Deals 3 Physical DMG.
 */
define skill {
  id 36012 as RockShieldwallDaPlama;
  skillType elemental;
  cost DiceType.Geo, 3;
  :damage(DamageType.Physical, 3);
};

/**
 * @id 36013
 * @name Aba Movo
 * @description
 * Deals 5 Physical DMG.
 */
define skill {
  id 36013 as RockShieldwallAbaMovo;
  skillType burst;
  cost DiceType.Geo, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Physical, 5);
};

/**
 * @id 36014
 * @name Craggy Greatshield
 * @description
 * (Passive) When the battle begins, this character gains Craggy Greatshield.
 */
define skill {
  id 36014 as CraggyGreatshieldPassive;
  skillType passive {
    on battleBegin {
      :characterStatus(CraggyGreatshield);
    };
  };
};

/**
 * @id 3601
 * @name Rock Shieldwall Mitachurl
 * @hp 8
 * @energy 2
 */
define character {
  id 3601 as RockShieldwallMitachurl;
  tags geo, monster, hilichurl;
  health 8;
  energy 2;
  skills RockShieldwallPlataPlama, RockShieldwallDaPlama, RockShieldwallAbaMovo, CraggyGreatshieldPassive;
};
