// Fatui Pyro Agent (4301): Tavern Challenge version of Piovium's 2301 with 9 HP.
// Every skill id (23011-23014) and Stealth (123011) match Piovium's implementation.
import {
  Thrust,
  Prowl,
  BladeAblaze,
  StealthMaster,
} from "../../characters/pyro/fatui_pyro_agent.gts";

define character {
  id 4301 as TavernFatuiPyroAgent;
  tags pyro, fatui;
  health 9;
  energy 2;
  skills Thrust, Prowl, BladeAblaze, StealthMaster;
};
