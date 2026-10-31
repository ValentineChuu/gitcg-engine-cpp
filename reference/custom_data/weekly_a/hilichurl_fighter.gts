// Hilichurl Fighter (3001). Damage values from the fandom wiki skill pages.
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 30011
 * @name Hit
 * @description
 * Deals 1 Physical DMG.
 */
define skill {
  id 30011 as Hit;
  skillType normal;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 1);
};

/**
 * @id 30012
 * @name Jump and Hit
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 30012 as JumpAndHit;
  skillType elemental;
  cost DiceType.Void, 3;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 30013
 * @name Swing That Club
 * @description
 * Deals 4 Physical DMG.
 */
define skill {
  id 30013 as SwingThatClub;
  skillType burst;
  cost DiceType.Aligned, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Physical, 4);
};

define character {
  id 3001 as HilichurlFighter;
  tags monster, hilichurl;
  health 4;
  energy 2;
  skills Hit, JumpAndHit, SwingThatClub;
};
