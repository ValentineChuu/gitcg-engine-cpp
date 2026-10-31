// Hydro Abyss Mage (3201). Damage values from the fandom wiki skill pages.
// The hidden passive 32015 Broken Ward is omitted: the status itself applies Stun when it breaks.
// Element instances follow Piovium's Elemental Lifeform: Hydro (hydro_tulpa.gts).
import { $, DamageType, DiceType } from "@gi-tcg/core/data";
import { Stun } from "./stun.gts";

/**
 * @id 132011
 * @name Mist Bubble
 * @description
 * The character cannot use Skills.
 * End Phase: Remove this status, deal 1 Hydro DMG to the character to which it is attached.
 */
define status {
  id 132011 as MistBubble;
  tags disableSkill;
  on endPhase {
    const master = :self.master;
    :dispose();
    :damage(DamageType.Hydro, 1, master);
  };
};

/**
 * @id 132012
 * @name Hydro Ward
 * @description
 * The character to which this is attached takes no DMG.
 * This status will provide 2 instances of Hydro Application (Can be depleted using Elemental Reactions):
 * Once the instances are depleted, this status will be removed, and the character to which this is
 * attached will be unable to use Skills until the end of this Round.
 */
define status {
  id 132012 as HydroWard;
  variable instances, 2;
  variable enabled, 0;
  on selfEnter {
    :apply(DamageType.Hydro, :self.master);
    :apply(DamageType.Hydro, :self.master);
    :setVariable("enabled", 1);
  };
  on modifyReaction {
    when :( :getVariable("enabled") && :getVariable("instances") > 0 );
    :addVariable("instances", -1);
    if (:getVariable("instances") > 0) {
      :apply(DamageType.Hydro, :self.master);
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
 * @id 32011
 * @name Water Ball
 * @description
 * Deals 1 Hydro DMG.
 */
define skill {
  id 32011 as WaterBall;
  skillType normal;
  cost DiceType.Hydro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Hydro, 1);
};

/**
 * @id 32012
 * @name Bubbling Offering
 * @description
 * Deals 1 Hydro DMG. The target character receives Mist Bubble.
 */
define skill {
  id 32012 as BubblingOffering;
  skillType elemental;
  cost DiceType.Hydro, 3;
  :damage(DamageType.Hydro, 1);
  :characterStatus(MistBubble, $.opp.active);
};

/**
 * @id 32013
 * @name Rite of Mortal Water
 * @description
 * Deals 4 Hydro DMG. The target character receives Mist Bubble.
 */
define skill {
  id 32013 as RiteOfMortalWater;
  skillType burst;
  cost DiceType.Hydro, 4;
  cost DiceType.Energy, 3;
  :damage(DamageType.Hydro, 4);
  :characterStatus(MistBubble, $.opp.active);
};

/**
 * @id 32014
 * @name Hydro Ward
 * @description
 * (Passive) When the battle begins, this character gains Hydro Ward.
 */
define skill {
  id 32014 as HydroWardPassive;
  skillType passive {
    on battleBegin {
      :characterStatus(HydroWard, :self);
    };
  };
};

define character {
  id 3201 as HydroAbyssMage;
  tags hydro, monster;
  health 4;
  energy 3;
  skills WaterBall, BubblingOffering, RiteOfMortalWater, HydroWardPassive;
};
