// Nobushi: Kikouban (3405): Tavern Challenge companion. Damage values from the fandom wiki skill pages.
import { DamageType, DiceType } from "@gi-tcg/core/data";
import { IaiSheathed, IaiThrust, IaiBreathing, Zanshin } from "./nobushi_iai.gts";

/**
 * @id 34051
 * @name Combo Slash
 * Deals 2 Physical DMG.
 */
define skill {
  id 34051 as ComboSlashElectro;
  skillType normal;
  cost DiceType.Electro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 34052
 * @name Mechanical Thunderbow
 * Deals 3 Electro DMG.
 */
define skill {
  id 34052 as MechanicalThunderbow;
  skillType elemental;
  cost DiceType.Electro, 3;
  :damage(DamageType.Electro, 3);
};

define character {
  id 3405 as NobushiKikouban;
  tags electro;
  health 6;
  energy 2;
  skills ComboSlashElectro, MechanicalThunderbow, IaiSheathed, IaiThrust, IaiBreathing, Zanshin;
};
