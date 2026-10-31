// Abyss Lector: Violet Lightning, Tavern Challenge boss (4409).
// Same attacks and Electric Rebirth as Piovium's 2406; triggering Electric Rebirth also raises Thunderous Ward.
import { DamageType } from "@gi-tcg/core/data";
import {
  ElectricRebirth,
  DenOfThunder,
  ShockOfTheEnigmaticAbyss,
  WildThunderburst,
} from "../../characters/electro/abyss_lector_violet_lightning.gts";

/**
 * @id 124064
 * @name Rolling Abyssal Thunder
 * @description
 * The character to which this is attached cannot use Skills.
 * End Phase: Deal 6 Piercing DMG to the character to which it is attached and remove the effect.
 */
define status {
  id 124064 as RollingAbyssalThunder;
  tags disableSkill;
  on endPhase {
    :damage(DamageType.Piercing, 6, :self.master);
    :dispose();
  };
};

/**
 * @id 124063
 * @name Thunderous Ward
 * @description
 * The character to which this is attached is immune to all DMG.
 * This status will provide 2 instances of Electro Application (Can be depleted using Elemental Reactions): Once the instances are depleted, this effect will be removed, and the character to which this is attached will be unable to use Skills and will take 6 Piercing DMG at the End Phase.
 */
define status {
  id 124063 as ThunderousWard;
  variable applications, 2;
  on selfEnter {
    :cleanAura("all", :self.master);
    :apply(DamageType.Electro, :self.master);
  };
  on decreaseDamaged {
    :e.decreaseDamage(:e.value);
  };
  on modifyReaction {
    :addVariable("applications", -1);
    if (:getVariable("applications") > 0) {
      :apply(DamageType.Electro, :self.master);
    } else {
      :characterStatus(RollingAbyssalThunder, :self.master);
      :dispose();
    }
  };
};

/**
 * @id 24065
 * @name Electric Rebirth
 * @description
 * When the battle begins, this character gains Electric Rebirth.
 */
define skill {
  id 24065 as ElectricRebirthTavern;
  skillType passive {
    on battleBegin {
      :characterStatus(ElectricRebirth);
    };
  };
};

/**
 * @id 24068
 * @name Electric Rebirth
 * @description
 * (Passive) When the battle begins, gain Electric Rebirth initially.
 * When this character triggers Electric Rebirth: They gain Thunderous Ward.
 * (The battle-begin half is already done by 24065, so this only handles the ward.)
 */
define skill {
  id 24068 as ElectricRebirthWard;
  skillType passive {
    // Electric Rebirth's only heal is its "will not be defeated" heal, so a heal from it means it triggered.
    on healed {
      when :( :e.damageInfo.via.caller.definition.id === ElectricRebirth );
      :characterStatus(ThunderousWard);
    };
  };
};

define character {
  id 4409 as AbyssLectorVioletLightningTavern;
  tags electro, monster;
  health 6;
  energy 2;
  skills DenOfThunder,
    ShockOfTheEnigmaticAbyss,
    WildThunderburst,
    ElectricRebirthTavern,
    ElectricRebirthWard;
};
