// Ice Shieldwall Mitachurl (3101). Damage values from the fandom wiki skill pages
// ("Plata Plama (Ice Shieldwall Mitachurl)" etc.): all three skills deal Physical DMG.
// The hidden passive 31015 Shattered Shield is omitted: removal on depletion is handled by the status.
// Element instances follow Piovium's Elemental Lifeform: Hydro (hydro_tulpa.gts): each Elemental Reaction
// on the character consumes one instance and re-applies Cryo while instances remain.
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 131011
 * @name Hoarfrost Greatshield
 * @description
 * The character to which this is attached takes -2 DMG.
 * This status will provide 2 instances of Cryo Application (Can be depleted using Elemental Reactions):
 * Once the instances are depleted, this status will be removed.
 */
define status {
  id 131011 as HoarfrostGreatshield;
  variable instances, 2;
  variable enabled, 0;
  on selfEnter {
    :apply(DamageType.Cryo, :self.master);
    :apply(DamageType.Cryo, :self.master);
    :setVariable("enabled", 1);
  };
  on modifyReaction {
    when :( :getVariable("enabled") && :getVariable("instances") > 0 );
    :addVariable("instances", -1);
    if (:getVariable("instances") > 0) {
      :apply(DamageType.Cryo, :self.master);
    }
  };
  on reaction {
    when :( :getVariable("enabled") && :getVariable("instances") <= 0 );
    :dispose();
  };
  on decreaseDamaged {
    :e.decreaseDamage(2);
  };
};

/**
 * @id 31011
 * @name Plata Plama
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 31011 as PlataPlama;
  skillType normal;
  cost DiceType.Cryo, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 31012
 * @name Da Plama
 * @description
 * Deals 3 Physical DMG.
 */
define skill {
  id 31012 as DaPlama;
  skillType elemental;
  cost DiceType.Cryo, 3;
  :damage(DamageType.Physical, 3);
};

/**
 * @id 31013
 * @name Aba Movo
 * @description
 * Deals 5 Physical DMG.
 */
define skill {
  id 31013 as AbaMovo;
  skillType burst;
  cost DiceType.Cryo, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Physical, 5);
};

/**
 * @id 31014
 * @name Hoarfrost Greatshield
 * @description
 * (Passive) When the battle begins, this character gains Hoarfrost Greatshield.
 */
define skill {
  id 31014 as HoarfrostGreatshieldPassive;
  skillType passive {
    on battleBegin {
      :characterStatus(HoarfrostGreatshield, :self);
    };
  };
};

define character {
  id 3101 as IceShieldwallMitachurl;
  tags cryo, monster, hilichurl;
  health 8;
  energy 2;
  skills PlataPlama, DaPlama, AbaMovo, HoarfrostGreatshieldPassive;
};
