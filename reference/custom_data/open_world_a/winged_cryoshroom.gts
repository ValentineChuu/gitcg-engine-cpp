// Winged Cryoshroom (3105): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages ("Spore Bullet (Winged Cryoshroom)": 1 Cryo,
// "Rolling Spray (Winged Cryoshroom)": 3 Cryo, "Divebomb (Winged Cryoshroom)": 5 Cryo).
// The states Aerial Superiority 137011, Scorched 137012 and Activated 137013 are shared with
// the Winged Dendroshroom and imported from its kit.
import { DamageType, DiceType } from "@gi-tcg/core/data";
import { AerialSuperiority } from "../weekly_b/winged_dendroshroom.gts";

/**
 * @id 31051
 * @name Spore Bullet
 * @description
 * Deals 1 Cryo DMG.
 */
define skill {
  id 31051 as WingedCryoshroomSporeBullet;
  skillType normal;
  cost DiceType.Cryo, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Cryo, 1);
};

/**
 * @id 31052
 * @name Rolling Spray
 * @description
 * Deals 3 Cryo DMG.
 */
define skill {
  id 31052 as WingedCryoshroomRollingSpray;
  skillType elemental;
  cost DiceType.Cryo, 3;
  :damage(DamageType.Cryo, 3);
};

/**
 * @id 31053
 * @name Divebomb
 * @description
 * Deals 5 Cryo DMG.
 */
define skill {
  id 31053 as WingedCryoshroomDivebomb;
  skillType burst;
  cost DiceType.Cryo, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Cryo, 5);
};

/**
 * @id 31054
 * @name State Change
 * @description
 * (Passive) When the battle begins, the character will be in the Aerial Superiority State.
 * After this character takes Pyro DMG, it will become Scorched.
 * After this character takes Electro DMG, it will become Activated.
 */
define skill {
  id 31054 as WingedCryoshroomStateChange;
  skillType passive {
    on battleBegin {
      :characterStatus(AerialSuperiority);
    };
  };
};

/**
 * @id 3105
 * @name Winged Cryoshroom
 * @hp 6
 * @energy 2
 */
define character {
  id 3105 as WingedCryoshroom;
  tags cryo, monster;
  health 6;
  energy 2;
  skills WingedCryoshroomSporeBullet, WingedCryoshroomRollingSpray, WingedCryoshroomDivebomb, WingedCryoshroomStateChange;
};
