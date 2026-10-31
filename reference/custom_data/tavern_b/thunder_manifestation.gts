// Thunder Manifestation, Tavern Challenge boss (4404 normal, 4405 hard).
// Same skills as Piovium's 2402, plus Elemental Sedimentation: Electro.
import {
  LightningRod,
  ThunderousWingslash,
  StrifefulLightning,
  ThunderingShackles,
  LightningProbe,
} from "../../characters/electro/thunder_manifestation.gts";
import { ElementalSedimentationElectro } from "./elemental_lifeform_electro.gts";

// Helper passive, not a game id: Piovium's Lightning Rod (124022) only boosts DMG whose source is
// character 2402 or its summon, so this gives the NPC ids the same "+1 DMG and remove Lightning Rod".
define skill {
  id 4404024 as LightningRodForNpcThunderManifestation;
  skillType passive {
    on increaseDamage {
      when :( !!:e.target.hasStatus(LightningRod) );
      :e.increaseDamage(1);
      :dispose(:e.target.hasStatus(LightningRod)!);
    };
  };
};

define character {
  id 4404 as ThunderManifestationNormal;
  tags electro, monster;
  health 30;
  energy 2;
  skills ThunderousWingslash,
    StrifefulLightning,
    ThunderingShackles,
    LightningProbe,
    ElementalSedimentationElectro,
    LightningRodForNpcThunderManifestation;
};

define character {
  id 4405 as ThunderManifestationHard;
  tags electro, monster;
  health 45;
  energy 2;
  skills ThunderousWingslash,
    StrifefulLightning,
    ThunderingShackles,
    LightningProbe,
    ElementalSedimentationElectro,
    LightningRodForNpcThunderManifestation;
};
