// Crackling Axe Mitachurl (3403): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages "Du Plama/Upa/Oto Tiga (Crackling Axe Mitachurl)":
// 2 / 3 / 5 Physical (the first 3 Physical hits become Electro through Ripple Electro Edge).
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 134031
 * @name Ripple Electro Edge
 * @description
 * The Physical DMG this character deals is converted to Electro DMG.
 * Usage(s): 3
 */
define status {
  id 134031 as RippleElectroEdge;
  on modifySkillDamageType {
    when :( :e.type === DamageType.Physical );
    usage 3;
    :e.changeDamageType(DamageType.Electro);
  };
};

/**
 * @id 34031
 * @name Du Plama
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 34031 as DuPlama;
  skillType normal;
  cost DiceType.Electro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 34032
 * @name Upa
 * @description
 * Deals 3 Physical DMG.
 */
define skill {
  id 34032 as CracklingAxeUpa;
  skillType elemental;
  cost DiceType.Electro, 3;
  :damage(DamageType.Physical, 3);
};

/**
 * @id 34033
 * @name Oto Tiga
 * @description
 * Deals 5 Physical DMG.
 */
define skill {
  id 34033 as OtoTiga;
  skillType burst;
  cost DiceType.Electro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Physical, 5);
};

/**
 * @id 34034
 * @name Ripple Electro Edge
 * @description
 * (Passive) When the battle begins, this character gains Ripple Electro Edge.
 */
define skill {
  id 34034 as RippleElectroEdgePassive;
  skillType passive {
    on battleBegin {
      :characterStatus(RippleElectroEdge);
    };
  };
};

/**
 * @id 3403
 * @name Crackling Axe Mitachurl
 * @hp 8
 * @energy 2
 */
define character {
  id 3403 as CracklingAxeMitachurl;
  tags electro, monster, hilichurl;
  health 8;
  energy 2;
  skills DuPlama, CracklingAxeUpa, OtoTiga, RippleElectroEdgePassive;
};
