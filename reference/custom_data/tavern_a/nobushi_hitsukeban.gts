// Nobushi: Hitsukeban (3305): Tavern Challenge companion. Damage values from the fandom wiki skill pages.
import { $, DamageType, DiceType } from "@gi-tcg/core/data";
import { IaiSheathed, IaiThrust, IaiBreathing, Zanshin } from "./nobushi_iai.gts";

/**
 * @id 33051
 * @name Combo Slash
 * Deals 2 Physical DMG.
 */
define skill {
  id 33051 as ComboSlashPyro;
  skillType normal;
  cost DiceType.Pyro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 33052
 * @name Blaze Bloom
 * Deals 3 Pyro DMG, switches your character to the next character.
 */
define skill {
  id 33052 as BlazeBloom;
  skillType elemental;
  cost DiceType.Pyro, 3;
  :damage(DamageType.Pyro, 3);
  :switchActive($.my.next);
};

define character {
  id 3305 as NobushiHitsukeban;
  tags pyro;
  health 6;
  energy 2;
  skills ComboSlashPyro, BlazeBloom, IaiSheathed, IaiThrust, IaiBreathing, Zanshin;
};
