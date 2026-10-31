// Hydro Hilichurl Rogue (4209): Tavern Challenge version of Piovium's 2205.
// Differs in its burst (22056): it equips Mist Bubble Slime on itself instead of creating the card in hand.
import { DamageType, DiceType } from "@gi-tcg/core/data";
import {
  WhirlingScythe,
  SlashOfSurgingTides,
  SlashOfSurgingTidesPassive,
  MistBubbleSlime,
} from "../../characters/hydro/hydro_hilichurl_rogue.gts";

/**
 * @id 22056
 * @name Bubblefloat Blitz
 * Deals 4 Hydro DMG. Attaches Mist Bubble Slime to this character.
 */
define skill {
  id 22056 as BubblefloatBlitzEquip;
  skillType burst;
  cost DiceType.Hydro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Hydro, 4);
  :equip(MistBubbleSlime, :self);
};

define character {
  id 4209 as TavernHydroHilichurlRogue;
  tags hydro, monster, hilichurl;
  health 10;
  energy 2;
  skills WhirlingScythe,
    SlashOfSurgingTides,
    BubblefloatBlitzEquip,
    SlashOfSurgingTidesPassive;
};
