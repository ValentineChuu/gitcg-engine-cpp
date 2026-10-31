// Abyss Lector: Fathomless Flames, Tavern Challenge boss (4303).
// Same attacks and Fiery Rebirth as Piovium's 2302; triggering Fiery Rebirth also raises Ward of Surging Flame.
import { DamageType } from "@gi-tcg/core/data";
import {
  FieryRebirthStatus,
  FlameOfSalvation,
  SearingPrecept,
  OminousStar,
  FieryRebirth,
  FieryRebirthSkill,
} from "../../characters/pyro/abyss_lector_fathomless_flames.gts";

/**
 * @id 123025
 * @name Quenched Embers
 * @description
 * The character to which this is attached cannot use Skills.
 * End Phase: Deal 6 Piercing DMG to the character to which it is attached and remove the effect.
 */
define status {
  id 123025 as QuenchedEmbers;
  tags disableSkill;
  on endPhase {
    :damage(DamageType.Piercing, 6, :self.master);
    :dispose();
  };
};

/**
 * @id 123023
 * @name Ward of Surging Flame
 * @description
 * The character to which this is attached is immune to all DMG.
 * This status will provide 2 instances of Pyro Application (Can be depleted using Elemental Reactions): Once the instances are depleted, this effect will be removed, and the character to which this is attached will be unable to use Skills and take 6 Piercing DMG at the End Phase.
 */
define status {
  id 123023 as WardOfSurgingFlame;
  variable applications, 2;
  on selfEnter {
    :cleanAura("all", :self.master);
    :apply(DamageType.Pyro, :self.master);
  };
  on decreaseDamaged {
    :e.decreaseDamage(:e.value);
  };
  on modifyReaction {
    :addVariable("applications", -1);
    if (:getVariable("applications") > 0) {
      :apply(DamageType.Pyro, :self.master);
    } else {
      :characterStatus(QuenchedEmbers, :self.master);
      :dispose();
    }
  };
};

/**
 * @id 23025
 * @name Ward of Surging Flame
 * @description
 * (Passive) When this character triggers Fiery Rebirth: attach Ward of Surging Flame to them.
 */
define skill {
  id 23025 as WardOfSurgingFlamePassive;
  skillType passive {
    // Fiery Rebirth's only heal is its "will not be defeated" heal, so a heal from it means it triggered.
    on healed {
      when :( :e.damageInfo.via.caller.definition.id === FieryRebirthStatus );
      :characterStatus(WardOfSurgingFlame);
    };
  };
};

define character {
  id 4303 as AbyssLectorFathomlessFlamesTavern;
  tags pyro, monster;
  health 6;
  energy 2;
  skills FlameOfSalvation,
    SearingPrecept,
    OminousStar,
    FieryRebirth,
    WardOfSurgingFlamePassive,
    FieryRebirthSkill;
};
