// Fatui Skirmisher - Electrohammer Vanguard (3407): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages "Rapid Hammer Strike": 2 Physical, "Raging Charge": 3 Electro,
// "Thunderblast Hammer": 3 Electro.
// Electro Elemental Equipment follows weekly_b's Pyro Elemental Equipment (Pyroslinger Bracer).
// Hidden passive 34075 "Shattered Gun" is not a skill here: its effect (no Skills for the rest of the Round
// once the equipment breaks) is the shared Stun status (100001).
import { Aura, DamageType, DiceType } from "@gi-tcg/core/data";
import { Stun } from "../weekly_a/stun.gts";

/**
 * @id 134071
 * @name Electro Elemental Equipment
 * @description
 * The character to which this is attached takes -1 DMG and deals +1 DMG.
 * This status will provide 2 instances of Electro Application (Can be depleted using Elemental Reactions):
 * Once the instances are depleted, this status will be removed, and the character to which this is
 * attached will be unable to use Skills until the end of this Round.
 */
define status {
  id 134071 as ElectroElementalEquipment;
  variable instances, 2;
  // The equipment replaces whatever element the character had with its own Electro.
  on selfEnter {
    if (:self.master.aura !== Aura.Electro) {
      if (:self.master.aura !== Aura.None) {
        :cleanAura("all", :self.master);
      }
      :apply(DamageType.Electro, :self.master);
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
      :characterStatus(Stun, :self.master);
      :dispose();
    } else if (:self.master.variables.alive && :self.master.aura === Aura.None) {
      :apply(DamageType.Electro, :self.master);
    }
  };
};

/**
 * @id 34071
 * @name Rapid Hammer Strike
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 34071 as RapidHammerStrike;
  skillType normal;
  cost DiceType.Electro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 34072
 * @name Raging Charge
 * @description
 * Deals 3 Electro DMG.
 */
define skill {
  id 34072 as RagingCharge;
  skillType elemental;
  cost DiceType.Electro, 3;
  :damage(DamageType.Electro, 3);
};

/**
 * @id 34073
 * @name Thunderblast Hammer
 * @description
 * Deals 3 Electro DMG. This character gains Electro Elemental Equipment.
 */
define skill {
  id 34073 as ThunderblastHammer;
  skillType burst;
  cost DiceType.Electro, 3;
  cost DiceType.Energy, 3;
  :damage(DamageType.Electro, 3);
  :characterStatus(ElectroElementalEquipment, :self);
};

/**
 * @id 34074
 * @name Combat Resupply
 * @description
 * (Passive) When the battle begins, this character gains 2 Energy.
 */
define skill {
  id 34074 as ElectrohammerCombatResupply;
  skillType passive {
    on battleBegin {
      :gainEnergy(2, :self);
    };
  };
};

/**
 * @id 3407
 * @name Fatui Skirmisher - Electrohammer Vanguard
 * @hp 10
 * @energy 3
 */
define character {
  id 3407 as FatuiSkirmisherElectrohammerVanguard;
  tags electro, fatui;
  health 10;
  energy 3;
  skills RapidHammerStrike, RagingCharge, ThunderblastHammer, ElectrohammerCombatResupply;
};
