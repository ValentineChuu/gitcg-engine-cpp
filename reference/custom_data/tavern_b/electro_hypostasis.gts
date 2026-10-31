// Electro Hypostasis, Tavern Challenge boss (4401 normal, 4402 hard).
// Same attacks as Piovium's 2401; the crystal core heals to 6 / 10 HP and can be regained at Action Phase start.
import {
  ElectroCrystalProjection,
  RockpaperscissorsCombo,
  LightningLockdown,
  RockpaperscissorsComboScissors,
  RockpaperscissorsComboPaper,
} from "../../characters/electro/electro_hypostasis.gts";
import { ElementalSedimentationElectro } from "./elemental_lifeform_electro.gts";

/**
 * @id 124015
 * @name Electro Crystal Core
 * @description
 * When the character to which this is attached would be defeated: Remove this effect, ensure the character will not be defeated, and heal them to 6 HP.
 */
define status {
  id 124015 as ElectroCrystalCoreSix;
  on beforeDefeated {
    :immune(6);
    :dispose();
  };
};

/**
 * @id 124016
 * @name Electro Crystal Core
 * @description
 * When the character to which this is attached would be defeated: Remove this effect, ensure the character will not be defeated, and heal them to 10 HP.
 */
define status {
  id 124016 as ElectroCrystalCoreTen;
  on beforeDefeated {
    :immune(10);
    :dispose();
  };
};

/**
 * @id 24017
 * @name Electro Crystal Core
 * @description
 * (Passive) When the battle begins, starts with Electro Crystal Core (124015).
 * When Action Phase begins: If this character has yet to gain Electro Crystal Core, then it will gain it again. (Triggers a maximum of twice in a single duel)
 */
define skill {
  id 24017 as ElectroCrystalCoreSixPassive;
  skillType passive {
    on battleBegin {
      :characterStatus(ElectroCrystalCoreSix);
    };
    on actionPhase {
      when :( !:self.hasStatus(ElectroCrystalCoreSix) );
      usage 2 {
        name "regainCoreUsage";
      };
      :characterStatus(ElectroCrystalCoreSix);
    };
  };
};

/**
 * @id 24018
 * @name Electro Crystal Core
 * @description
 * (Passive) When the battle begins, starts with Electro Crystal Core (124016).
 * When Action Phase begins: If this character has yet to gain Electro Crystal Core, then it will gain it again. (Triggers a maximum of twice in a single duel)
 */
define skill {
  id 24018 as ElectroCrystalCoreTenPassive;
  skillType passive {
    on battleBegin {
      :characterStatus(ElectroCrystalCoreTen);
    };
    on actionPhase {
      when :( !:self.hasStatus(ElectroCrystalCoreTen) );
      usage 2 {
        name "regainCoreUsage";
      };
      :characterStatus(ElectroCrystalCoreTen);
    };
  };
};

define character {
  id 4401 as ElectroHypostasisNormal;
  tags electro, monster;
  health 30;
  energy 2;
  skills ElectroCrystalProjection,
    RockpaperscissorsCombo,
    LightningLockdown,
    ElectroCrystalCoreSixPassive,
    RockpaperscissorsComboScissors,
    RockpaperscissorsComboPaper,
    ElementalSedimentationElectro;
};

define character {
  id 4402 as ElectroHypostasisHard;
  tags electro, monster;
  health 45;
  energy 2;
  skills ElectroCrystalProjection,
    RockpaperscissorsCombo,
    LightningLockdown,
    ElectroCrystalCoreTenPassive,
    RockpaperscissorsComboScissors,
    RockpaperscissorsComboPaper,
    ElementalSedimentationElectro;
};
