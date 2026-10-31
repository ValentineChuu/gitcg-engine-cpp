// Hilichurl Berserker (3304): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages (Torch Strike, Hit and Run, Scorching Combo).
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 33041
 * @name Torch Strike
 * @description
 * Deals 1 Pyro DMG.
 */
define skill {
  id 33041 as TorchStrike;
  skillType normal;
  cost DiceType.Pyro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Pyro, 1);
};

/**
 * @id 33042
 * @name Hit and Run
 * @description
 * Deals 2 Pyro DMG.
 */
define skill {
  id 33042 as HitAndRun;
  skillType elemental;
  cost DiceType.Pyro, 2;
  :damage(DamageType.Pyro, 2);
};

/**
 * @id 33043
 * @name Scorching Combo
 * @description
 * Deals 4 Pyro DMG.
 */
define skill {
  id 33043 as ScorchingCombo;
  skillType burst;
  cost DiceType.Pyro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Pyro, 4);
};

/**
 * @id 3304
 * @name Hilichurl Berserker
 * @hp 4
 * @energy 2
 */
define character {
  id 3304 as HilichurlBerserker;
  tags pyro, monster, hilichurl;
  health 4;
  energy 2;
  skills TorchStrike, HitAndRun, ScorchingCombo;
};
