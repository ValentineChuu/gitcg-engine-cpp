// Wooden Shieldwall Mitachurl (3702): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages "Plata Plama/Da Plama/Aba Movo (Wooden Shieldwall Mitachurl)":
// 2 / 3 / 5 Physical.
// Crudewood Greatshield follows weekly_a's Hoarfrost Greatshield (Ice Shieldwall Mitachurl).
// The hidden passive 37025 Shattered Shield is omitted: removal on depletion is handled by the status.
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 137021
 * @name Crudewood Greatshield
 * @description
 * The character to which this is attached takes -2 DMG.
 * This status will provide 2 instances of Dendro Application (Can be depleted using Elemental Reactions):
 * Once the instances are depleted, this status will be removed.
 */
define status {
  id 137021 as CrudewoodGreatshield;
  variable instances, 2;
  variable enabled, 0;
  on selfEnter {
    :apply(DamageType.Dendro, :self.master);
    :apply(DamageType.Dendro, :self.master);
    :setVariable("enabled", 1);
  };
  on modifyReaction {
    when :( :getVariable("enabled") && :getVariable("instances") > 0 );
    :addVariable("instances", -1);
    if (:getVariable("instances") > 0) {
      :apply(DamageType.Dendro, :self.master);
    }
  };
  on reaction {
    when :( :getVariable("enabled") && :getVariable("instances") <= 0 );
    :dispose();
  };
  on decreaseDamaged {
    :e.decreaseDamage(Math.min(2, :e.value));
  };
};

/**
 * @id 37021
 * @name Plata Plama
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 37021 as WoodenShieldwallPlataPlama;
  skillType normal;
  cost DiceType.Dendro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 37022
 * @name Da Plama
 * @description
 * Deals 3 Physical DMG.
 */
define skill {
  id 37022 as WoodenShieldwallDaPlama;
  skillType elemental;
  cost DiceType.Dendro, 3;
  :damage(DamageType.Physical, 3);
};

/**
 * @id 37023
 * @name Aba Movo
 * @description
 * Deals 5 Physical DMG.
 */
define skill {
  id 37023 as WoodenShieldwallAbaMovo;
  skillType burst;
  cost DiceType.Dendro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Physical, 5);
};

/**
 * @id 37024
 * @name Crudewood Greatshield
 * @description
 * (Passive) When the battle begins, this character gains Crudewood Greatshield.
 */
define skill {
  id 37024 as CrudewoodGreatshieldPassive;
  skillType passive {
    on battleBegin {
      :characterStatus(CrudewoodGreatshield, :self);
    };
  };
};

/**
 * @id 3702
 * @name Wooden Shieldwall Mitachurl
 * @hp 8
 * @energy 2
 */
define character {
  id 3702 as WoodenShieldwallMitachurl;
  tags dendro, monster, hilichurl;
  health 8;
  energy 2;
  skills WoodenShieldwallPlataPlama, WoodenShieldwallDaPlama, WoodenShieldwallAbaMovo, CrudewoodGreatshieldPassive;
};
