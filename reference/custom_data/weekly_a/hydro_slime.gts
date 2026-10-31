// Hydro Slime (3204). Damage values from the fandom wiki skill pages.
// Elemental Lifeform: Hydro copies Piovium's 222062 (hydro_tulpa.gts) without its 2-round duration.
import { Aura, DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 132041
 * @name Elemental Lifeform: Hydro
 * @description
 * This character always has Hydro applied, and it is immune to Hydro DMG.
 */
define status {
  id 132041 as SlimeElementalLifeformHydro;
  variable enableModification, 0;
  on selfEnter {
    :apply(DamageType.Hydro, :self.master);
    :apply(DamageType.Hydro, :self.master);
    :setVariable("enableModification", +(:self.master.aura === Aura.Hydro));
  };
  on modifyReaction {
    when :( :getVariable("enableModification") );
    :apply(DamageType.Hydro, :self.master);
  };
  on decreaseDamaged {
    when :( :e.type === DamageType.Hydro );
    :e.decreaseDamage(:e.value);
  };
};

/**
 * @id 32041
 * @name Bounce
 * @description
 * Deals 1 Hydro DMG.
 */
define skill {
  id 32041 as Bounce;
  skillType normal;
  cost DiceType.Hydro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Hydro, 1);
};

/**
 * @id 32042
 * @name Slam
 * @description
 * Deals 4 Hydro DMG.
 */
define skill {
  id 32042 as Slam;
  skillType burst;
  cost DiceType.Hydro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Hydro, 4);
};

/**
 * @id 32043
 * @name Elemental Sedimentation: Hydro
 * @description
 * (Passive) When the battle begins, this character gains Elemental Lifeform: Hydro.
 */
define skill {
  id 32043 as ElementalSedimentationHydro;
  skillType passive {
    on battleBegin {
      :characterStatus(SlimeElementalLifeformHydro, :self);
    };
  };
};

define character {
  id 3204 as HydroSlime;
  tags hydro, monster;
  health 4;
  energy 2;
  skills Bounce, Slam, ElementalSedimentationHydro;
};
