// Large Cryo Slime (3108): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages "Bounce (Large Cryo Slime)": 1 Cryo, "Frigid Spray": 3 Cryo,
// "Slam (Large Cryo Slime)": 5 Cryo.
// The passive 31073 and its status 131071 are shared with the Tavern Cryo Hypostasis (tavern_a).
import { DamageType, DiceType } from "@gi-tcg/core/data";
import { ElementalSedimentationCryo } from "../tavern_a/cryo_hypostasis.gts";

/**
 * @id 31081
 * @name Bounce
 * @description
 * Deals 1 Cryo DMG.
 */
define skill {
  id 31081 as LargeCryoSlimeBounce;
  skillType normal;
  cost DiceType.Cryo, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Cryo, 1);
};

/**
 * @id 31082
 * @name Frigid Spray
 * @description
 * Deals 3 Cryo DMG.
 */
define skill {
  id 31082 as FrigidSpray;
  skillType elemental;
  cost DiceType.Cryo, 3;
  :damage(DamageType.Cryo, 3);
};

/**
 * @id 31083
 * @name Slam
 * @description
 * Deals 5 Cryo DMG.
 */
define skill {
  id 31083 as LargeCryoSlimeSlam;
  skillType burst;
  cost DiceType.Cryo, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Cryo, 5);
};

/**
 * @id 3108
 * @name Large Cryo Slime
 * @hp 10
 * @energy 2
 */
define character {
  id 3108 as LargeCryoSlime;
  tags cryo, monster;
  health 10;
  energy 2;
  skills LargeCryoSlimeBounce, FrigidSpray, LargeCryoSlimeSlam, ElementalSedimentationCryo;
};
