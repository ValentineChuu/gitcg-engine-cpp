// Pyro Abyss Mage, open-world version (5301): weekly_a's Pyro Abyss Mage (3301) with 6 HP and the
// extra passive Aegis of the Abyss. Skills 33011-33014 and Pyro Ward keep the same ids and descriptions
// as 3301, so they are imported from weekly_a. The hidden passive 33015 Broken Ward is omitted there too.
// Aegis of the Abyss has no usage limit in its description: the Ward is restored after every Skill used
// while it is missing. Any other element on the mage is cleared first, so the restored Ward's Pyro
// does not react with it.
import { Aura } from "@gi-tcg/core/data";
import {
  Flamewave,
  BlazingVisage,
  RiteOfTheMoltenEruption,
  PyroWardPassive,
  PyroWard,
} from "../weekly_a/pyro_abyss_mage.gts";

/**
 * @id 33016
 * @name Aegis of the Abyss
 * @description
 * (Passive) When the Ward is broken, after this character uses a Skill: Restore the Pyro Ward.
 */
define skill {
  id 33016 as PyroAegisOfTheAbyss;
  skillType passive {
    on useSkill {
      when :( !:self.hasStatus(PyroWard) );
      if (:self.aura !== Aura.None) {
        :cleanAura("all", :self);
      }
      :characterStatus(PyroWard, :self);
    };
  };
};

/**
 * @id 5301
 * @name Pyro Abyss Mage
 * @hp 6
 * @energy 3
 */
define character {
  id 5301 as PyroAbyssMageOpenWorld;
  tags pyro, monster;
  health 6;
  energy 3;
  skills Flamewave, BlazingVisage, RiteOfTheMoltenEruption, PyroWardPassive, PyroAegisOfTheAbyss;
};
