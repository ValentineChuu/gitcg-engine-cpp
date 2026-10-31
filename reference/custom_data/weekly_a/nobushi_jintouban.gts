// Nobushi: Jintouban (3003). Damage values from the fandom wiki skill pages.
// Iai: Sheathed prepares for 2 turns: the first prepared turn uses Iai: Breathing (no effect),
// the second uses Iai: Thrust, mirroring Piovium's chained prepare statuses (nextStatus).
import { DamageType, DiceType } from "@gi-tcg/core/data";
import { IaiSheathed, IaiThrust, IaiBreathing, Zanshin } from "../tavern_a/nobushi_iai.gts";

/**
 * @id 30031
 * @name Combo Slash
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 30031 as ComboSlash;
  skillType normal;
  cost DiceType.Void, 3;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 30032
 * @name Leap Attack
 * @description
 * Deals 3 Physical DMG.
 */
define skill {
  id 30032 as LeapAttack;
  skillType elemental;
  cost DiceType.Aligned, 3;
  :damage(DamageType.Physical, 3);
};

define character {
  id 3003 as NobushiJintouban;
  health 6;
  energy 2;
  skills ComboSlash, LeapAttack, IaiSheathed, IaiThrust, IaiBreathing, Zanshin;
};
