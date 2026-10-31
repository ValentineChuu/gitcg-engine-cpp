// Fatui Skirmisher - Anemoboxer Vanguard (3502): NPC-only monster written from the game description.
// Damage values: Dimbreath declared values (Galeforce Agitation: 3 Anemo, Avenging Squall: 2, element
// left blank) and fandom wiki skill pages (Hurricane Jab: 2 Physical, Avenging Squall: 2 Physical).
// Hidden passive 35024 "Stormwind Barrier" is not a skill here: the barrier status implements it.
import { $, DamageType, DiceType } from "@gi-tcg/core/data";

const ABSORBABLE_TYPES: DamageType[] = [DamageType.Hydro, DamageType.Pyro, DamageType.Cryo, DamageType.Electro];

/**
 * @id 35025
 * @name Avenging Squall
 * @description
 * (Prepare for 1 turn)
 * Deals 2 Physical DMG.
 */
define skill {
  id 35025 as AvengingSquall;
  skillType elemental;
  prepared;
  :damage(DamageType.Physical, 2);
  // Prepared skills emit no useSkill event, so the barrier (which converts this DMG) ends here.
  const barrier = :self.hasStatus(StormwindBarrier);
  if (barrier) {
    :dispose(barrier);
  }
};

/**
 * @id 135021
 * @name Stormwind Barrier
 * @description
 * The next time this character acts, they will immediately use the Skill Avenging Squall.
 * While preparing this Skill: Grant the character to which this is attached 2 Shield points. Meanwhile,
 * if the character takes Hydro/Pyro/Cryo/Electro DMG, Avenging Squall will deal +1 DMG and its DMG type
 * will be converted to the corresponding Element as well.
 */
define status {
  id 135021 as StormwindBarrier;
  tags preparingSkill, shield;
  variable absorbedType, DamageType.Physical {
    visible false;
  };
  variable shield, 2;
  // Written by hand instead of the `shield` attribute, which would end the preparation once broken.
  on decreaseDamaged {
    when :( :getVariable("shield") > 0 && :e.value > 0 );
    const decreased = Math.min(:getVariable("shield"), :e.value);
    :e.decreaseDamage(decreased);
    :addVariable("shield", -decreased);
  };
  on damaged {
    when :( ABSORBABLE_TYPES.includes(:e.type) );
    :setVariable("absorbedType", :e.type);
  };
  on replaceActionBySkill {
    :useSkill(AvengingSquall, { asPrepared: true });
  };
  on modifySkillDamageType {
    when :(
      :e.via.definition.id === AvengingSquall &&
        :getVariable("absorbedType") !== DamageType.Physical
    );
    :e.changeDamageType(:getVariable("absorbedType") as DamageType);
  };
  on increaseSkillDamage {
    when :(
      :e.via.definition.id === AvengingSquall &&
        :getVariable("absorbedType") !== DamageType.Physical
    );
    :e.increaseDamage(1);
  };
  on switchActive {
    when :( :e.switchInfo.from?.id === :self.master.id );
    :dispose();
  };
};

/**
 * @id 35021
 * @name Hurricane Jab
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 35021 as HurricaneJab;
  skillType normal;
  cost DiceType.Anemo, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 35022
 * @name Gathering the Winds
 * @description
 * This character gains a Stormwind Barrier. Prepares Avenging Squall.
 */
define skill {
  id 35022 as GatheringTheWinds;
  skillType elemental;
  cost DiceType.Anemo, 3;
  :characterStatus(StormwindBarrier, :self);
};

/**
 * @id 35023
 * @name Galeforce Agitation
 * @description
 * Deals 3 Anemo DMG. All of your other Fatui characters gain 1 Energy.
 */
define skill {
  id 35023 as GaleforceAgitation;
  skillType burst;
  cost DiceType.Anemo, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Anemo, 3);
  for (const teammate of :queryAll($.my.character.tag("fatui"))) {
    if (teammate.id !== :self.id) {
      teammate.gainEnergy(1);
    }
  }
};

/**
 * @id 3502
 * @name Fatui Skirmisher - Anemoboxer Vanguard
 * @hp 10
 * @energy 2
 */
define character {
  id 3502 as FatuiSkirmisherAnemoboxerVanguard;
  tags anemo, fatui;
  health 10;
  energy 2;
  skills HurricaneJab, GatheringTheWinds, GaleforceAgitation, AvengingSquall;
};
