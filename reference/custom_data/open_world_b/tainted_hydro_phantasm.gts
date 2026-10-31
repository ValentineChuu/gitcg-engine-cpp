// Tainted Hydro Phantasm (3209): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages "Tainted Water Strike": 1 Hydro, "Spouting Waves": 2 Hydro,
// "Tainted Water Surge": 4 Hydro.
// Elemental Lifeform: Hydro (132041) is imported from weekly_a's Hydro Slime. tavern_a's Rhodeia also defines
// 132041, so the passive sets the Hydro aura itself, which works with either definition.
// "+2 DMG" is applied to this character's Skill DMG.
import { $, Aura, DamageType, DiceType, Reaction } from "@gi-tcg/core/data";
import { Frozen } from "../../commons.gts";
import { SlimeElementalLifeformHydro } from "../weekly_a/hydro_slime.gts";

/**
 * @id 32091
 * @name Tainted Water Strike
 * @description
 * Deals 1 Hydro DMG.
 */
define skill {
  id 32091 as TaintedWaterStrike;
  skillType normal;
  cost DiceType.Hydro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Hydro, 1);
};

/**
 * @id 32092
 * @name Spouting Waves
 * @description
 * Deals 2 Hydro DMG.
 */
define skill {
  id 32092 as SpoutingWaves;
  skillType elemental;
  cost DiceType.Hydro, 3;
  :damage(DamageType.Hydro, 2);
};

/**
 * @id 32093
 * @name Tainted Water Surge
 * @description
 * Deals 4 Hydro DMG.
 */
define skill {
  id 32093 as TaintedWaterSurge;
  skillType burst;
  cost DiceType.Hydro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Hydro, 4);
};

/**
 * @id 32094
 * @name Elemental Sedimentation: Hydro
 * @description
 * (Passive) When the battle begins, this character gains Elemental Lifeform: Hydro.
 */
define skill {
  id 32094 as TaintedElementalSedimentationHydro;
  skillType passive {
    on battleBegin {
      :characterStatus(SlimeElementalLifeformHydro, :self);
      if (:self.aura !== Aura.Hydro) {
        :self.setVariable("aura", Aura.Hydro);
      }
    };
  };
};

/**
 * @id 32095
 * @name Tainted Waters Change Their Shape
 * @description
 * (Passive) Will not be Frozen;
 * This character gains +2 DMG when their HP does not exceed 5.
 * After this character uses a Skill: If this character's HP is at least 6, deal 1 Piercing DMG to all
 * opposing standby characters.
 */
define skill {
  id 32095 as TaintedWatersChangeTheirShape;
  skillType passive {
    on reaction {
      when :( :e.type === Reaction.Frozen );
      :dispose($.typeStatus.def(Frozen).at(:self));
    };
    on increaseSkillDamage {
      when :( :self.health <= 5 );
      :e.increaseDamage(2);
    };
    on useSkill {
      when :( :self.health >= 6 );
      :damage(DamageType.Piercing, 1, $.opp.standby);
    };
  };
};

/**
 * @id 3209
 * @name Tainted Hydro Phantasm
 * @hp 10
 * @energy 2
 */
define character {
  id 3209 as TaintedHydroPhantasm;
  tags hydro, monster;
  health 10;
  energy 2;
  skills TaintedWaterStrike, SpoutingWaves, TaintedWaterSurge, TaintedElementalSedimentationHydro,
    TaintedWatersChangeTheirShape;
};
