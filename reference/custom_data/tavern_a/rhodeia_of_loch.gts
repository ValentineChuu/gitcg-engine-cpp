// Rhodeia of Loch (4201 normal, 4202 hard): Tavern Challenge boss versions of Piovium's 2201.
// Same active skills, plus Elemental Sedimentation: Hydro (permanent Hydro aura, immune to Hydro DMG).
import { ElementalSedimentationHydro } from "../weekly_a/hydro_slime.gts";
import {
  Surge,
  OceanidMimicSummoning,
  TheMyriadWilds,
  TideAndTorrent,
} from "../../characters/hydro/rhodeia_of_loch.gts";

define character {
  id 4201 as TavernRhodeiaOfLoch;
  tags hydro, monster;
  health 30;
  energy 3;
  skills Surge, OceanidMimicSummoning, TheMyriadWilds, TideAndTorrent, ElementalSedimentationHydro;
};

define character {
  id 4202 as TavernRhodeiaOfLochHard;
  tags hydro, monster;
  health 45;
  energy 3;
  skills Surge, OceanidMimicSummoning, TheMyriadWilds, TideAndTorrent, ElementalSedimentationHydro;
};
