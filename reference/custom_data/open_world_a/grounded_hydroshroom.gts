// Grounded Hydroshroom (3205): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages ("Spore Bullet (Grounded Hydroshroom)": 1 Hydro,
// "Raging Breath (Grounded Hydroshroom)": 3 Hydro, "Charging Blast (Grounded Hydroshroom)": 5 Hydro).
// Terrain Advantage 132051 is defined here and shared with the Grounded Geoshroom.
// Scorched 137012 and Activated 137013 are imported from the Winged Dendroshroom kit.
import { DamageType, DiceType } from "@gi-tcg/core/data";
import { Activated, Scorched } from "../weekly_b/winged_dendroshroom.gts";

/**
 * @id 132051
 * @name Terrain Advantage
 * @description
 * The character deals +1 DMG. (This character will permanently change its State if they take Pyro DMG or
 * Electro DMG.)
 */
define status {
  id 132051 as TerrainAdvantage;
  on increaseSkillDamage {
    :e.increaseDamage(1);
  };
  on damaged {
    when :( :e.type === DamageType.Pyro || :e.type === DamageType.Electro );
    if (:e.type === DamageType.Pyro) {
      :characterStatus(Scorched, :self.master);
    } else {
      :characterStatus(Activated, :self.master);
    }
    :dispose();
  };
};

/**
 * @id 32051
 * @name Spore Bullet
 * @description
 * Deals 1 Hydro DMG.
 */
define skill {
  id 32051 as GroundedHydroshroomSporeBullet;
  skillType normal;
  cost DiceType.Hydro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Hydro, 1);
};

/**
 * @id 32052
 * @name Raging Breath
 * @description
 * Deals 3 Hydro DMG.
 */
define skill {
  id 32052 as GroundedHydroshroomRagingBreath;
  skillType elemental;
  cost DiceType.Hydro, 3;
  :damage(DamageType.Hydro, 3);
};

/**
 * @id 32053
 * @name Charging Blast
 * @description
 * Deals 5 Hydro DMG.
 */
define skill {
  id 32053 as GroundedHydroshroomChargingBlast;
  skillType burst;
  cost DiceType.Hydro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Hydro, 5);
};

/**
 * @id 32054
 * @name State Change
 * @description
 * (Passive) When the battle begins, the character will be in the Terrain Advantage State.
 * After this character takes Pyro DMG, it will become Scorched.
 * After this character takes Electro DMG, it will become Activated.
 */
define skill {
  id 32054 as GroundedHydroshroomStateChange;
  skillType passive {
    on battleBegin {
      :characterStatus(TerrainAdvantage);
    };
  };
};

/**
 * @id 3205
 * @name Grounded Hydroshroom
 * @hp 7
 * @energy 2
 */
define character {
  id 3205 as GroundedHydroshroom;
  tags hydro, monster;
  health 7;
  energy 2;
  skills GroundedHydroshroomSporeBullet, GroundedHydroshroomRagingBreath, GroundedHydroshroomChargingBlast, GroundedHydroshroomStateChange;
};
