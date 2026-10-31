// Cryo Hilichurl Shooter (3102). Damage values from the fandom wiki skill pages.
import { $, DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 31021
 * @name Ice Arrow
 * @description
 * Deals 1 Cryo DMG.
 */
define skill {
  id 31021 as IceArrow;
  skillType normal;
  cost DiceType.Cryo, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Cryo, 1);
};

/**
 * @id 31022
 * @name Multishot
 * @description
 * Deals 4 Cryo DMG.
 */
define skill {
  id 31022 as CryoMultishot;
  skillType burst;
  cost DiceType.Cryo, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Cryo, 4);
};

/**
 * @id 31023
 * @name Hide
 * @description
 * (Passive) Switches to the next character after this character attacks.
 */
define skill {
  id 31023 as CryoHide;
  skillType passive {
    on useSkill {
      :switchActive($.my.next);
    };
  };
};

define character {
  id 3102 as CryoHilichurlShooter;
  tags cryo, monster, hilichurl;
  health 3;
  energy 2;
  skills IceArrow, CryoMultishot, CryoHide;
};
