// Elemental Sedimentation: Electro (34063) and Elemental Lifeform: Electro (134061),
// shared by the Tavern Challenge Electro Hypostasis and Thunder Manifestation kits.
// Modeled on Piovium's Elemental Lifeform: Hydro (222062, hydro_tulpa.gts), without its duration.
import { Aura, DamageType } from "@gi-tcg/core/data";

/**
 * @id 134061
 * @name Elemental Lifeform: Electro
 * @description
 * This character always has Electro applied, and it is immune to Electro DMG.
 */
define status {
  id 134061 as ElementalLifeformElectro;
  variable enableModification, 0;
  on selfEnter {
    :apply(DamageType.Electro, :self.master);
    :apply(DamageType.Electro, :self.master);
    :setVariable("enableModification", +(:self.master.aura === Aura.Electro));
  };
  on modifyReaction {
    when :( :getVariable("enableModification") );
    :apply(DamageType.Electro, :self.master);
  };
  on decreaseDamaged {
    when :( :e.type === DamageType.Electro );
    :e.decreaseDamage(:e.value);
  };
};

/**
 * @id 34063
 * @name Elemental Sedimentation: Electro
 * @description
 * (Passive) When the battle begins, this character gains Elemental Lifeform: Electro.
 */
define skill {
  id 34063 as ElementalSedimentationElectro;
  skillType passive {
    on battleBegin {
      :characterStatus(ElementalLifeformElectro);
    };
  };
};
