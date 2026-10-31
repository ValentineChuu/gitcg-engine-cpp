// Fatui Skirmisher - Pyroslinger Bracer (3306): NPC-only monster written from the game description.
// Damage values: Dimbreath declared values (Flaming Cannon: 3 Pyro) and fandom wiki skill pages
// (Single-Shot Bullet: 1 Pyro, Burning Barrage: 3 Pyro).
// Hidden passive 33065 "Shattered Gun" is not a skill here: its effect (no Skills for the rest of the
// Round once the equipment breaks) is the status below, on the unused id 133062.
import { Aura, DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 133062
 * @name Shattered Gun
 * @description
 * Character cannot use Skills. (Lasts until the end of this Round)
 */
define status {
  id 133062 as ShatteredGun;
  oneDuration;
  tags disableSkill;
};

/**
 * @id 133061
 * @name Pyro Elemental Equipment
 * @description
 * The character to which this is attached takes -1 DMG and deals +1 DMG.
 * This status will provide 2 instances of Pyro Application (Can be depleted using Elemental Reactions):
 * Once the instances are depleted, this status will be removed, and the character to which this is
 * attached will be unable to use Skills until the end of this Round.
 */
define status {
  id 133061 as PyroElementalEquipment;
  variable instances, 2;
  // The equipment replaces whatever element the character had with its own Pyro.
  on selfEnter {
    if (:self.master.aura !== Aura.Pyro) {
      if (:self.master.aura !== Aura.None) {
        :cleanAura("all", :self.master);
      }
      :apply(DamageType.Pyro, :self.master);
    }
  };
  on decreaseDamaged {
    when :( :e.value > 0 );
    :e.decreaseDamage(1);
  };
  on increaseSkillDamage {
    :e.increaseDamage(1);
  };
  on reaction {
    :addVariable("instances", -1);
    if (:getVariable("instances") <= 0) {
      :characterStatus(ShatteredGun, :self.master);
      :dispose();
    } else if (:self.master.variables.alive && :self.master.aura === Aura.None) {
      :apply(DamageType.Pyro, :self.master);
    }
  };
};

/**
 * @id 33061
 * @name Single-Shot Bullet
 * @description
 * Deals 1 Pyro DMG.
 */
define skill {
  id 33061 as SingleShotBullet;
  skillType normal;
  cost DiceType.Pyro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Pyro, 1);
};

/**
 * @id 33062
 * @name Burning Barrage
 * @description
 * Deals 3 Pyro DMG.
 */
define skill {
  id 33062 as BurningBarrage;
  skillType elemental;
  cost DiceType.Pyro, 3;
  :damage(DamageType.Pyro, 3);
};

/**
 * @id 33063
 * @name Flaming Cannon
 * @description
 * Deals 3 Pyro DMG. This character gains Pyro Elemental Equipment.
 */
define skill {
  id 33063 as FlamingCannon;
  skillType burst;
  cost DiceType.Pyro, 3;
  cost DiceType.Energy, 3;
  :damage(DamageType.Pyro, 3);
  :characterStatus(PyroElementalEquipment, :self);
};

/**
 * @id 33064
 * @name Combat Resupply
 * @description
 * (Passive) When the battle begins, this character gains 2 Energy.
 */
define skill {
  id 33064 as CombatResupply;
  skillType passive {
    on battleBegin {
      :gainEnergy(2, :self);
    };
  };
};

/**
 * @id 3306
 * @name Fatui Skirmisher - Pyroslinger Bracer
 * @hp 10
 * @energy 3
 */
define character {
  id 3306 as FatuiSkirmisherPyroslingerBracer;
  tags pyro, fatui;
  health 10;
  energy 3;
  skills SingleShotBullet, BurningBarrage, FlamingCannon, CombatResupply;
};
