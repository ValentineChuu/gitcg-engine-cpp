// Fatui Skirmisher - Cryogunner Legionnaire (3104). Damage values from the fandom wiki skill pages.
// The hidden passive 31045 Shattered Gun is omitted: the status itself applies Stun when it breaks.
// Element instances follow Piovium's Elemental Lifeform: Hydro (hydro_tulpa.gts).
import { DamageType, DiceType } from "@gi-tcg/core/data";
import { Stun } from "./stun.gts";

/**
 * @id 131041
 * @name Cryo Elemental Equipment
 * @description
 * The character to which this is attached takes -1 DMG and deals +1 DMG.
 * This status will provide 2 instances of Cryo Application (Can be depleted using Elemental Reactions):
 * Once the instances are depleted, this status will be removed, and the character to which this is
 * attached will be unable to use Skills until the end of this Round.
 */
define status {
  id 131041 as CryoElementalEquipment;
  variable instances, 2;
  variable enabled, 0;
  on selfEnter {
    :apply(DamageType.Cryo, :self.master);
    :apply(DamageType.Cryo, :self.master);
    :setVariable("enabled", 1);
  };
  on modifyReaction {
    // While applying its own element on entry, reactions with an existing aura have no side effects
    // (e.g. the character does not Freeze itself).
    when :( !:getVariable("enabled") );
    :e.cancelCoreEffects();
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
    :e.decreaseDamage(1);
  };
  on increaseSkillDamage {
    :e.increaseDamage(1);
  };
};

/**
 * @id 31041
 * @name Close-Range Charge
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 31041 as CloseRangeCharge;
  skillType normal;
  cost DiceType.Cryo, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 31042
 * @name Freezing Spray
 * @description
 * Deals 3 Cryo DMG.
 */
define skill {
  id 31042 as FreezingSpray;
  skillType elemental;
  cost DiceType.Cryo, 3;
  :damage(DamageType.Cryo, 3);
};

/**
 * @id 31043
 * @name Frost Cannon
 * @description
 * Deals 3 Cryo DMG. This character gains Cryo Elemental Equipment.
 */
define skill {
  id 31043 as FrostCannon;
  skillType burst;
  cost DiceType.Cryo, 3;
  cost DiceType.Energy, 3;
  :damage(DamageType.Cryo, 3);
  :characterStatus(CryoElementalEquipment, :self);
};

/**
 * @id 31044
 * @name Combat Resupply
 * @description
 * (Passive) When the battle begins, this character gains 2 Energy.
 */
define skill {
  id 31044 as CryogunnerCombatResupply;
  skillType passive {
    on battleBegin {
      :gainEnergy(2, :self);
    };
  };
};

define character {
  id 3104 as FatuiCryogunnerLegionnaire;
  tags cryo, fatui;
  health 10;
  energy 3;
  skills CloseRangeCharge, FreezingSpray, FrostCannon, CryogunnerCombatResupply;
};
