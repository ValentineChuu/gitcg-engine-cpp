// Hydro Abyss Mage, open-world version (5201): weekly_a's Hydro Abyss Mage (3201) with 6 HP and the
// extra passive Aegis of the Abyss. Skills 32011-32014 and the statuses keep the same ids and descriptions
// as 3201, so they are imported from weekly_a. The hidden passive 32015 Broken Ward is omitted there too.
// Aegis of the Abyss has no usage limit in its description: the Ward is restored after every Skill used
// while it is missing. Any other element on the mage is cleared first, so the restored Ward's Hydro
// does not react with it.
import { Aura } from "@gi-tcg/core/data";
import {
  WaterBall,
  BubblingOffering,
  RiteOfMortalWater,
  HydroWardPassive,
  HydroWard,
} from "../weekly_a/hydro_abyss_mage.gts";

/**
 * @id 32016
 * @name Aegis of the Abyss
 * @description
 * (Passive) When the Ward is broken, after this character uses a Skill: Restore the Hydro Ward.
 */
define skill {
  id 32016 as HydroAegisOfTheAbyss;
  skillType passive {
    on useSkill {
      when :( !:self.hasStatus(HydroWard) );
      if (:self.aura !== Aura.None) {
        :cleanAura("all", :self);
      }
      :characterStatus(HydroWard, :self);
    };
  };
};

/**
 * @id 5201
 * @name Hydro Abyss Mage
 * @hp 6
 * @energy 3
 */
define character {
  id 5201 as HydroAbyssMageOpenWorld;
  tags hydro, monster;
  health 6;
  energy 3;
  skills WaterBall, BubblingOffering, RiteOfMortalWater, HydroWardPassive, HydroAegisOfTheAbyss;
};
