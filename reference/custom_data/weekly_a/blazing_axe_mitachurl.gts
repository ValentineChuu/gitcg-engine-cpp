// Blazing Axe Mitachurl (3302). Damage values from the fandom wiki skill pages
// ("Upa (Blazing Axe Mitachurl)" etc.): all three skills deal Physical DMG, converted by Flowfire Edge.
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 133021
 * @name Flowfire Edge
 * @description
 * The Physical DMG this character deals is converted to Pyro DMG.
 * Usage(s): 3
 */
define status {
  id 133021 as FlowfireEdge;
  on modifySkillDamageType {
    when :( :e.type === DamageType.Physical );
    usage 3;
    :e.changeDamageType(DamageType.Pyro);
  };
};

/**
 * @id 33021
 * @name Du Plama
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 33021 as DuPlama;
  skillType normal;
  cost DiceType.Pyro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 33022
 * @name Upa
 * @description
 * Deals 3 Physical DMG.
 */
define skill {
  id 33022 as Upa;
  skillType elemental;
  cost DiceType.Pyro, 3;
  :damage(DamageType.Physical, 3);
};

/**
 * @id 33023
 * @name Oto Tiga
 * @description
 * Deals 5 Physical DMG.
 */
define skill {
  id 33023 as OtoTiga;
  skillType burst;
  cost DiceType.Pyro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Physical, 5);
};

/**
 * @id 33024
 * @name Flowfire Edge
 * @description
 * (Passive) When the battle begins, this character gains Flowfire Edge.
 */
define skill {
  id 33024 as FlowfireEdgePassive;
  skillType passive {
    on battleBegin {
      :characterStatus(FlowfireEdge, :self);
    };
  };
};

define character {
  id 3302 as BlazingAxeMitachurl;
  tags pyro, monster, hilichurl;
  health 8;
  energy 2;
  skills DuPlama, Upa, OtoTiga, FlowfireEdgePassive;
};
