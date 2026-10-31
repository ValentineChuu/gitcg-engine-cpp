// Hilichurl Shooter (3002). Damage values from the fandom wiki skill pages.
import { $, DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 30021
 * @name Shooting
 * @description
 * Deals 1 Physical DMG.
 */
define skill {
  id 30021 as Shooting;
  skillType normal;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 1);
};

/**
 * @id 30022
 * @name Multishot
 * @description
 * Deals 5 Physical DMG.
 */
define skill {
  id 30022 as Multishot;
  skillType burst;
  cost DiceType.Aligned, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Physical, 5);
};

/**
 * @id 30023
 * @name Hide
 * @description
 * (Passive) Switches to the next character after this character uses a Skill.
 */
define skill {
  id 30023 as Hide;
  skillType passive {
    on useSkill {
      :switchActive($.my.next);
    };
  };
};

define character {
  id 3002 as HilichurlShooter;
  tags monster, hilichurl;
  health 3;
  energy 2;
  skills Shooting, Multishot, Hide;
};
