// Pyro Abyss Mage (3301). Damage values from the fandom wiki skill pages.
// The hidden passive 33015 Broken Ward is omitted: the status itself applies Stun when it breaks.
// Element instances follow Piovium's Elemental Lifeform: Hydro (hydro_tulpa.gts).
import { DamageType, DiceType } from "@gi-tcg/core/data";
import { Stun } from "./stun.gts";

/**
 * @id 133011
 * @name Pyro Ward
 * @description
 * The character to which this is attached takes no DMG.
 * This status will provide 2 instances of Pyro Application (Can be depleted using Elemental Reactions):
 * Once the instances are depleted, this status will be removed, and the character to which this is
 * attached will be unable to use Skills until the end of this Round.
 */
define status {
  id 133011 as PyroWard;
  variable instances, 2;
  variable enabled, 0;
  on selfEnter {
    :apply(DamageType.Pyro, :self.master);
    :apply(DamageType.Pyro, :self.master);
    :setVariable("enabled", 1);
  };
  on modifyReaction {
    when :( :getVariable("enabled") && :getVariable("instances") > 0 );
    :addVariable("instances", -1);
    if (:getVariable("instances") > 0) {
      :apply(DamageType.Pyro, :self.master);
    }
  };
  on reaction {
    when :( :getVariable("enabled") && :getVariable("instances") <= 0 );
    const master = :self.master;
    :dispose();
    :characterStatus(Stun, master);
  };
  on decreaseDamaged {
    :e.decreaseDamage(:e.value);
  };
};

/**
 * @id 33011
 * @name Flamewave
 * @description
 * Deals 1 Pyro DMG.
 */
define skill {
  id 33011 as Flamewave;
  skillType normal;
  cost DiceType.Pyro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Pyro, 1);
};

/**
 * @id 33012
 * @name Blazing Visage
 * @description
 * Deals 3 Pyro DMG.
 */
define skill {
  id 33012 as BlazingVisage;
  skillType elemental;
  cost DiceType.Pyro, 3;
  :damage(DamageType.Pyro, 3);
};

/**
 * @id 33013
 * @name Rite of the Molten Eruption
 * @description
 * Deals 7 Pyro DMG.
 */
define skill {
  id 33013 as RiteOfTheMoltenEruption;
  skillType burst;
  cost DiceType.Pyro, 4;
  cost DiceType.Energy, 3;
  :damage(DamageType.Pyro, 7);
};

/**
 * @id 33014
 * @name Pyro Ward
 * @description
 * (Passive) When the battle begins, this character gains Pyro Ward.
 */
define skill {
  id 33014 as PyroWardPassive;
  skillType passive {
    on battleBegin {
      :characterStatus(PyroWard, :self);
    };
  };
};

define character {
  id 3301 as PyroAbyssMage;
  tags pyro, monster;
  health 4;
  energy 3;
  skills Flamewave, BlazingVisage, RiteOfTheMoltenEruption, PyroWardPassive;
};
