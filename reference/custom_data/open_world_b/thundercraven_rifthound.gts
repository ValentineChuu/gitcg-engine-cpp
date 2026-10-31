// Thundercraven Rifthound (3409): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages "Cankering Claws": 1 Electro, "Corrosive Electro Crystal": 2 Electro,
// "Devouring Bite": 6 Electro. Corrosion (136021) is shared with the Rockfond Rifthound (corrosion.gts).
import { DamageType, DiceType } from "@gi-tcg/core/data";
import { Corrosion } from "./corrosion.gts";

/**
 * @id 134091
 * @name Elemental Devourer
 * @description
 * After taking Electro DMG, stack 1 level of Elemental Devourer. (Max 2 levels)
 * When 2 levels of Elemental Devourer have been gained, character deals +1 Electro DMG and takes +2 Electro DMG.
 */
define status {
  id 134091 as ElectroElementalDevourer;
  variable level, 0;
  on damaged {
    when :( :e.type === DamageType.Electro && :getVariable("level") < 2 );
    :addVariable("level", 1);
  };
  on increaseSkillDamage {
    when :( :getVariable("level") >= 2 && :e.type === DamageType.Electro );
    :e.increaseDamage(1);
  };
  on increaseDamaged {
    when :( :getVariable("level") >= 2 && :e.type === DamageType.Electro );
    :e.increaseDamage(2);
  };
};

/**
 * @id 34091
 * @name Cankering Claws
 * @description
 * Deals 1 Electro DMG, creates 1 Corrosion on the opponent's side of the field.
 */
define skill {
  id 34091 as CankeringClaws;
  skillType normal;
  cost DiceType.Electro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Electro, 1);
  :combatStatus(Corrosion, "opp");
};

/**
 * @id 34092
 * @name Corrosive Electro Crystal
 * @description
 * Deals 2 Electro DMG, creates 1 Corrosion on the opponent's side of the field.
 */
define skill {
  id 34092 as CorrosiveElectroCrystal;
  skillType elemental;
  cost DiceType.Electro, 3;
  :damage(DamageType.Electro, 2);
  :combatStatus(Corrosion, "opp");
};

/**
 * @id 34093
 * @name Devouring Bite
 * @description
 * Deals 6 Electro DMG, creates 1 Corrosion on the opponent's side of the field.
 */
define skill {
  id 34093 as DevouringBite;
  skillType burst;
  cost DiceType.Electro, 3;
  cost DiceType.Energy, 3;
  :damage(DamageType.Electro, 6);
  :combatStatus(Corrosion, "opp");
};

/**
 * @id 34094
 * @name Elemental Devourer
 * @description
 * (Passive) When the battle begins, this character gains Elemental Devourer.
 */
define skill {
  id 34094 as ElectroElementalDevourerPassive;
  skillType passive {
    on battleBegin {
      :characterStatus(ElectroElementalDevourer, :self);
    };
  };
};

/**
 * @id 3409
 * @name Thundercraven Rifthound
 * @hp 10
 * @energy 3
 */
define character {
  id 3409 as ThundercravenRifthound;
  tags electro, monster;
  health 10;
  energy 3;
  skills CankeringClaws, CorrosiveElectroCrystal, DevouringBite, ElectroElementalDevourerPassive;
};
