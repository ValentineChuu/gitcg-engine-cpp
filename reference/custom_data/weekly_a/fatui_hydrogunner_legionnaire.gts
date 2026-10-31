// Fatui Skirmisher - Hydrogunner Legionnaire (3203). Damage values from the fandom wiki skill pages.
// The hidden passive 32035 Shattered Gun is omitted: the status itself applies Stun when it breaks.
// Element instances follow Piovium's Elemental Lifeform: Hydro (hydro_tulpa.gts).
// Piovium has no "increase healing" event, so the +1 Healing of Hydro Elemental Equipment is applied by
// Curative Bullet (this character's only healing) when the status is present.
import { $, DamageType, DiceType } from "@gi-tcg/core/data";
import { Stun } from "./stun.gts";

/**
 * @id 132031
 * @name Hydro Elemental Equipment
 * @description
 * The character to which this is attached takes -1 DMG and deals +1 Healing.
 * This status will provide 2 instances of Hydro Application (Can be depleted using Elemental Reactions):
 * Once the instances are depleted, this status will be removed, and the character to which this is
 * attached will be unable to use Skills until the end of this Round.
 */
define status {
  id 132031 as HydroElementalEquipment;
  variable instances, 2;
  variable enabled, 0;
  on selfEnter {
    :apply(DamageType.Hydro, :self.master);
    :apply(DamageType.Hydro, :self.master);
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
    :e.decreaseDamage(1);
  };
};

/**
 * @id 32031
 * @name Skilled Charge
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 32031 as SkilledCharge;
  skillType normal;
  cost DiceType.Hydro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 32032
 * @name Curative Bullet
 * @description
 * Deals 2 Hydro DMG, heals 2 of friendly characters on standby for 1 HP.
 */
define skill {
  id 32032 as CurativeBullet;
  skillType elemental;
  cost DiceType.Hydro, 3;
  :damage(DamageType.Hydro, 2);
  :heal(:self.hasStatus(HydroElementalEquipment) ? 2 : 1, $.my.standby);
};

/**
 * @id 32033
 * @name Elemental Buff: Hydro
 * @description
 * Deals 3 Hydro DMG. This character gains Hydro Elemental Equipment.
 */
define skill {
  id 32033 as ElementalBuffHydro;
  skillType burst;
  cost DiceType.Hydro, 3;
  cost DiceType.Energy, 3;
  :damage(DamageType.Hydro, 3);
  :characterStatus(HydroElementalEquipment, :self);
};

/**
 * @id 32034
 * @name Combat Resupply
 * @description
 * (Passive) When the battle begins, this character gains 2 Energy.
 */
define skill {
  id 32034 as HydrogunnerCombatResupply;
  skillType passive {
    on battleBegin {
      :gainEnergy(2, :self);
    };
  };
};

define character {
  id 3203 as FatuiHydrogunnerLegionnaire;
  tags hydro, fatui;
  health 10;
  energy 3;
  skills SkilledCharge, CurativeBullet, ElementalBuffHydro, HydrogunnerCombatResupply;
};
