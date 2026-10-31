// Electro Hilichurl Shooter (3402): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages (Thunder Arrow: 1 Electro, Multishot: 5 Physical).
import { $, DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 34021
 * @name Thunder Arrow
 * @description
 * Deals 1 Electro DMG.
 */
define skill {
  id 34021 as ThunderArrow;
  skillType normal;
  cost DiceType.Electro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Electro, 1);
};

/**
 * @id 34022
 * @name Multishot
 * @description
 * Deals 5 Physical DMG.
 */
define skill {
  id 34022 as Multishot;
  skillType burst;
  cost DiceType.Electro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Physical, 5);
};

/**
 * @id 34023
 * @name Hide
 * @description
 * (Passive) Switches to the next character after this character attacks.
 */
define skill {
  id 34023 as Hide;
  skillType passive {
    on useSkill {
      :switchActive($.my.next);
    };
  };
};

/**
 * @id 3402
 * @name Electro Hilichurl Shooter
 * @hp 3
 * @energy 2
 */
define character {
  id 3402 as ElectroHilichurlShooter;
  tags electro, monster, hilichurl;
  health 3;
  energy 2;
  skills ThunderArrow, Multishot, Hide;
};
