// Cryo Abyss Mage (3103). Damage values from the fandom wiki skill pages.
// The hidden passive 31035 Broken Ward is omitted: the status itself applies Stun when it breaks.
// Element instances follow Piovium's Elemental Lifeform: Hydro (hydro_tulpa.gts); the status is removed
// after the reaction that consumes the last instance resolves, so that hit still deals no DMG.
// "Cannot be Frozen" uses the immuneControl tag (blocks disableSkill statuses while the ward is up).
import { DamageType, DiceType } from "@gi-tcg/core/data";
import { Stun } from "./stun.gts";

/**
 * @id 131031
 * @name Cryo Ward
 * @description
 * The character to which this is attached takes no DMG and cannot be Frozen.
 * This status will provide 2 instances of Cryo Infusion (Can be depleted using Elemental Reactions):
 * Once the instances are depleted, this status will be removed, and the character to which this is
 * attached will be unable to use Skills until the end of this Round.
 */
define status {
  id 131031 as CryoWard;
  tags immuneControl;
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
    const master = :self.master;
    :dispose();
    :characterStatus(Stun, master);
  };
  on decreaseDamaged {
    :e.decreaseDamage(:e.value);
  };
};

/**
 * @id 31031
 * @name Cryo Thorns
 * @description
 * Deals 1 Cryo DMG.
 */
define skill {
  id 31031 as CryoThorns;
  skillType normal;
  cost DiceType.Cryo, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Cryo, 1);
};

/**
 * @id 31032
 * @name Frostdrift Offering
 * @description
 * Deals 3 Cryo DMG.
 */
define skill {
  id 31032 as FrostdriftOffering;
  skillType elemental;
  cost DiceType.Cryo, 3;
  :damage(DamageType.Cryo, 3);
};

/**
 * @id 31033
 * @name Rimestorm Rite
 * @description
 * Deals 5 Cryo DMG.
 */
define skill {
  id 31033 as RimestormRite;
  skillType burst;
  cost DiceType.Cryo, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Cryo, 5);
};

/**
 * @id 31034
 * @name Cryo Ward
 * @description
 * (Passive) When the battle begins, this character gains Cryo Ward.
 */
define skill {
  id 31034 as CryoWardPassive;
  skillType passive {
    on battleBegin {
      :characterStatus(CryoWard, :self);
    };
  };
};

define character {
  id 3103 as CryoAbyssMage;
  tags cryo, monster;
  health 4;
  energy 2;
  skills CryoThorns, FrostdriftOffering, RimestormRite, CryoWardPassive;
};
