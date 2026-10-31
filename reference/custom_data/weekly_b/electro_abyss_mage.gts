// Electro Abyss Mage (3404): NPC-only monster written from the game description.
// Damage values: Dimbreath declared values (Rite of Rolling Thunder: 4 Electro) and fandom wiki pages
// (Thunderbolt: 1 Electro, Tanglebolt Ring: 2 Electro x2).
// Hidden passive 34045 "Broken Ward" is not a skill here: its effect (no Skills for the rest of the
// Round once the ward breaks) is the status below, on the unused id 134043.
import { Aura, DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 134043
 * @name Broken Ward
 * @description
 * Character cannot use Skills. (Lasts until the end of this Round)
 */
define status {
  id 134043 as BrokenWard;
  oneDuration;
  tags disableSkill;
};

/**
 * @id 134042
 * @name Electro Ward
 * @description
 * The character to which this is attached takes no DMG.
 * This status will provide 2 instances of Electro Infusion (Can be depleted using Elemental Reactions):
 * Once the instances are depleted, this status will be removed, and the character to which this is
 * attached will be unable to use Skills until the end of this Round.
 */
define status {
  id 134042 as ElectroWard;
  tags barrier;
  variable instances, 2;
  // The ward replaces whatever element the character had with its own Electro.
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
    :e.decreaseDamage(:e.value);
  };
  on reaction {
    :addVariable("instances", -1);
    if (:getVariable("instances") <= 0) {
      :characterStatus(BrokenWard, :self.master);
      :dispose();
    } else if (:self.master.variables.alive && :self.master.aura === Aura.None) {
      :apply(DamageType.Electro, :self.master);
    }
  };
};

/**
 * @id 134041
 * @name Tanglebolt Ring
 * @description
 * End Phase: Deal 2 Electro DMG.
 * Usage(s): 2
 */
define summon {
  id 134041 as TangleboltRing;
  hint DamageType.Electro, 2;
  on endPhase {
    usage 2;
    :damage(DamageType.Electro, 2);
  };
};

/**
 * @id 34041
 * @name Thunderbolt
 * @description
 * Deals 1 Electro DMG.
 */
define skill {
  id 34041 as Thunderbolt;
  skillType normal;
  cost DiceType.Electro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Electro, 1);
};

/**
 * @id 34042
 * @name Lingering Light
 * @description
 * Summons 1 Tanglebolt Ring.
 */
define skill {
  id 34042 as LingeringLight;
  skillType elemental;
  cost DiceType.Electro, 3;
  :summon(TangleboltRing);
};

/**
 * @id 34043
 * @name Rite of Rolling Thunder
 * @description
 * Deals 4 Electro DMG, summons 1 Tanglebolt Ring.
 */
define skill {
  id 34043 as RiteOfRollingThunder;
  skillType burst;
  cost DiceType.Electro, 3;
  cost DiceType.Energy, 3;
  :damage(DamageType.Electro, 4);
  :summon(TangleboltRing);
};

/**
 * @id 34044
 * @name Electro Ward
 * @description
 * (Passive) When the battle begins, this character gains Electro Ward.
 */
define skill {
  id 34044 as ElectroWardPassive;
  skillType passive {
    on battleBegin {
      :characterStatus(ElectroWard);
    };
  };
};

/**
 * @id 3404
 * @name Electro Abyss Mage
 * @hp 4
 * @energy 3
 */
define character {
  id 3404 as ElectroAbyssMage;
  tags electro, monster;
  health 4;
  energy 3;
  skills Thunderbolt, LingeringLight, RiteOfRollingThunder, ElectroWardPassive;
};
