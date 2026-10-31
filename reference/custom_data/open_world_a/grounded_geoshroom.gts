// Grounded Geoshroom (3605): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages ("Spore Bullet (Grounded Geoshroom)": 1 Geo,
// "Raging Breath (Grounded Geoshroom)": 3 Geo, "Charging Blast (Grounded Geoshroom)": 5 Geo).
// Terrain Advantage 132051 is shared with the Grounded Hydroshroom and imported from its kit.
import { DamageType, DiceType } from "@gi-tcg/core/data";
import { TerrainAdvantage } from "./grounded_hydroshroom.gts";

/**
 * @id 36051
 * @name Spore Bullet
 * @description
 * Deals 1 Geo DMG.
 */
define skill {
  id 36051 as GroundedGeoshroomSporeBullet;
  skillType normal;
  cost DiceType.Geo, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Geo, 1);
};

/**
 * @id 36052
 * @name Raging Breath
 * @description
 * Deals 3 Geo DMG.
 */
define skill {
  id 36052 as GroundedGeoshroomRagingBreath;
  skillType elemental;
  cost DiceType.Geo, 3;
  :damage(DamageType.Geo, 3);
};

/**
 * @id 36053
 * @name Charging Blast
 * @description
 * Deals 5 Geo DMG.
 */
define skill {
  id 36053 as GroundedGeoshroomChargingBlast;
  skillType burst;
  cost DiceType.Geo, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Geo, 5);
};

/**
 * @id 36054
 * @name State Change
 * @description
 * (Passive) When the battle begins, the character will be in the Terrain Advantage State.
 * After this character takes Pyro DMG, it will become Scorched.
 * After this character takes Electro DMG, it will become Activated.
 */
define skill {
  id 36054 as GroundedGeoshroomStateChange;
  skillType passive {
    on battleBegin {
      :characterStatus(TerrainAdvantage);
    };
  };
};

/**
 * @id 3605
 * @name Grounded Geoshroom
 * @hp 7
 * @energy 2
 */
define character {
  id 3605 as GroundedGeoshroom;
  tags geo, monster;
  health 7;
  energy 2;
  skills GroundedGeoshroomSporeBullet, GroundedGeoshroomRagingBreath, GroundedGeoshroomChargingBlast, GroundedGeoshroomStateChange;
};
