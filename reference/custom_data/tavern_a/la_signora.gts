// La Signora (4103 normal, 4104 hard): Tavern Challenge boss versions of Piovium's 2102.
// Same active skills; the passive grants a stronger Ice-Sealed status (121023 / 121024) that breaks at
// 15 / 25 HP instead of 4. Piovium only reserves those two ids, so they are defined here.
// Piovium's Inferno's Awakening (21025) only reacts to the removal of 121021, so these statuses perform the
// transformation into Crimson Witch of Embers themselves, wherever they are removed.
import {
  FrostbladeHailstorm,
  BitingShards,
  CarmineChrysalis,
  InfernosAwakening,
} from "../../characters/cryo/la_signora.gts";
import { CrimsonWitchOfEmbers } from "../../characters/pyro/crimson_witch_of_embers.gts";

/**
 * @id 121023
 * @name Ice-Sealed Crimson Witch of Embers
 * Action Phase start: if the character has no more than 15 HP, remove this effect.
 * When the character is defeated: remove this effect, prevent the defeat and heal to 1 HP.
 * When removed: the character transforms into Crimson Witch of Embers.
 */
define status {
  id 121023 as IceSealedCrimsonWitchNormal;
  on actionPhase {
    when :( :self.master.health <= 15 );
    :transformDefinition(:self.master, CrimsonWitchOfEmbers);
    :dispose();
  };
  on beforeDefeated {
    :immune(1);
    :transformDefinition(:self.master, CrimsonWitchOfEmbers);
    :dispose();
  };
  on useSkill {
    when :( :e.skill.definition.id === CarmineChrysalis );
    :transformDefinition(:self.master, CrimsonWitchOfEmbers);
    :dispose();
  };
};

/**
 * @id 121024
 * @name Ice-Sealed Crimson Witch of Embers
 * Same as 121023, but removed at the Action Phase start when the character has no more than 25 HP.
 */
define status {
  id 121024 as IceSealedCrimsonWitchHard;
  on actionPhase {
    when :( :self.master.health <= 25 );
    :transformDefinition(:self.master, CrimsonWitchOfEmbers);
    :dispose();
  };
  on beforeDefeated {
    :immune(1);
    :transformDefinition(:self.master, CrimsonWitchOfEmbers);
    :dispose();
  };
  on useSkill {
    when :( :e.skill.definition.id === CarmineChrysalis );
    :transformDefinition(:self.master, CrimsonWitchOfEmbers);
    :dispose();
  };
};

/**
 * @id 21026
 * @name Might of Delusion
 * (Passive) When the battle begins, this character gains Ice-Sealed Crimson Witch of Embers (121023).
 */
define skill {
  id 21026 as MightOfDelusionNormal;
  skillType passive {
    on battleBegin {
      :characterStatus(IceSealedCrimsonWitchNormal);
    };
    on revive {
      :characterStatus(IceSealedCrimsonWitchNormal);
    };
  };
};

/**
 * @id 21027
 * @name Might of Delusion
 * (Passive) When the battle begins, this character gains Ice-Sealed Crimson Witch of Embers (121024).
 */
define skill {
  id 21027 as MightOfDelusionHard;
  skillType passive {
    on battleBegin {
      :characterStatus(IceSealedCrimsonWitchHard);
    };
    on revive {
      :characterStatus(IceSealedCrimsonWitchHard);
    };
  };
};

define character {
  id 4103 as TavernLaSignora;
  tags cryo, fatui;
  health 30;
  energy 2;
  skills FrostbladeHailstorm,
    BitingShards,
    CarmineChrysalis,
    MightOfDelusionNormal,
    InfernosAwakening;
};

define character {
  id 4104 as TavernLaSignoraHard;
  tags cryo, fatui;
  health 50;
  energy 2;
  skills FrostbladeHailstorm,
    BitingShards,
    CarmineChrysalis,
    MightOfDelusionHard,
    InfernosAwakening;
};
