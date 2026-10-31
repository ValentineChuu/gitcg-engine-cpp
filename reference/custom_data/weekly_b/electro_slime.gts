// Electro Slime (3406): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages "Bounce (Electro Slime)", "Slam (Electro Slime)".
// Passive 34063 (Elemental Sedimentation: Electro) and status 134061 (Elemental Lifeform: Electro) are
// shared with the Electro Hypostasis / Thunder Manifestation kits and imported from tavern_b.
import { DamageType, DiceType } from "@gi-tcg/core/data";
import { ElementalSedimentationElectro } from "../tavern_b/elemental_lifeform_electro.gts";

/**
 * @id 34061
 * @name Bounce
 * @description
 * Deals 1 Electro DMG.
 */
define skill {
  id 34061 as Bounce;
  skillType normal;
  cost DiceType.Electro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Electro, 1);
};

/**
 * @id 34062
 * @name Slam
 * @description
 * Deals 4 Electro DMG.
 */
define skill {
  id 34062 as Slam;
  skillType burst;
  cost DiceType.Electro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Electro, 4);
};

/**
 * @id 3406
 * @name Electro Slime
 * @hp 4
 * @energy 2
 */
define character {
  id 3406 as ElectroSlime;
  tags electro, monster;
  health 4;
  energy 2;
  skills Bounce, Slam, ElementalSedimentationElectro;
};
