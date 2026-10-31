// Eremite Scorching Loremaster, Tavern Challenge companion (4305).
// Same kit as Piovium's 2303, except its burst equips Spirit of Omen: Pyro Scorpion directly (once per match).
import { DamageType, DiceType } from "@gi-tcg/core/data";
import {
  SpiritOfOmenPyroScorpion,
  SearingGlare,
  BlazingStrike,
  SpiritOfOmensPower,
} from "../../characters/pyro/eremite_scorching_loremaster.gts";

/**
 * @id 23038
 * @name Spirit of Omen's Awakening: Pyro Scorpion
 * @cost 3*Pyro, 2*Energy
 * @description
 * Deals 3 Pyro DMG. Once each Match, attaches Spirit of Omen: Pyro Scorpion to this character.
 */
define skill {
  id 23038 as SpiritOfOmensAwakeningPyroScorpionTavern;
  skillType burst;
  cost DiceType.Pyro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Pyro, 3);
  // Spirit of Omen's Power (23034) declares the once-per-match counter "createCardUsage" on this
  // character for the PvP burst; the NPC burst spends that same counter.
  if (:self.getVariable("createCardUsage") > 0) {
    :self.setVariable("createCardUsage", 0);
    :equip(SpiritOfOmenPyroScorpion, :self);
  }
};

define character {
  id 4305 as EremiteScorchingLoremasterTavern;
  tags pyro, eremite;
  health 10;
  energy 2;
  skills SearingGlare,
    BlazingStrike,
    SpiritOfOmensAwakeningPyroScorpionTavern,
    SpiritOfOmensPower;
};
