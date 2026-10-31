// Eremite Floral Ring-Dancer, Tavern Challenge companion (4707).
// Same kit as Piovium's 2703, except its burst equips Spirit of Omen: Dendro Spirit-Serpent directly (once per match).
import { DamageType, DiceType } from "@gi-tcg/core/data";
import {
  SpiritOfOmenDendroSpiritserpent,
  FloralringCaress,
  SpiralingWhirl,
  SpiritOfOmensPower,
} from "../../characters/dendro/eremite_floral_ringdancer.gts";

/**
 * @id 27035
 * @name Spirit of Omen's Awakening: Dendro Spirit-Serpent
 * @cost 3*Dendro, 2*Energy
 * @description
 * Deals 4 Dendro DMG. Once each Match, attaches Spirit of Omen: Dendro Spirit-Serpent to this character.
 */
define skill {
  id 27035 as SpiritOfOmensAwakeningDendroSpiritserpentTavern;
  skillType burst;
  cost DiceType.Dendro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Dendro, 4);
  // Spirit of Omen's Power (27034) declares the once-per-match counter "createCardUsage" on this
  // character for the PvP burst; the NPC burst spends that same counter.
  if (:self.getVariable("createCardUsage") > 0) {
    :self.setVariable("createCardUsage", 0);
    :equip(SpiritOfOmenDendroSpiritserpent, :self);
  }
};

define character {
  id 4707 as EremiteFloralRingdancerTavern;
  tags dendro, eremite;
  health 10;
  energy 2;
  skills FloralringCaress,
    SpiralingWhirl,
    SpiritOfOmensAwakeningDendroSpiritserpentTavern,
    SpiritOfOmensPower;
};
