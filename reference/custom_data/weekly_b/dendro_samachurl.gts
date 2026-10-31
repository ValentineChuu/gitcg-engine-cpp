// Dendro Samachurl (3703): NPC-only monster written from the game description.
// Damage values: Dimbreath declared values (Venomous Vines: 1 Dendro) and fandom wiki skill pages
// ("Staff Strike (Dendro Samachurl)": 1 Physical, Dendro Field: 5 Dendro).
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 137031
 * @name Vines
 * @description
 * After a character switch takes place in this play area: Deal 1 Dendro DMG to the character activated.
 * Usage(s): 2
 */
define combatStatus {
  id 137031 as Vines;
  on switchActive {
    usage 2;
    :damage(DamageType.Dendro, 1, :e.switchInfo.to);
  };
};

/**
 * @id 37031
 * @name Staff Strike
 * @description
 * Deals 1 Physical DMG.
 */
define skill {
  id 37031 as DendroSamachurlStaffStrike;
  skillType normal;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 1);
};

/**
 * @id 37032
 * @name Venomous Vines
 * @description
 * Deals 1 Dendro DMG, creates 1 Vines at the opponent's play area.
 */
define skill {
  id 37032 as VenomousVines;
  skillType elemental;
  cost DiceType.Dendro, 3;
  :damage(DamageType.Dendro, 1);
  :combatStatus(Vines, "opp");
};

/**
 * @id 37033
 * @name Dendro Field
 * @description
 * Deals 5 Dendro DMG.
 */
define skill {
  id 37033 as DendroField;
  skillType burst;
  cost DiceType.Dendro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Dendro, 5);
};

/**
 * @id 3703
 * @name Dendro Samachurl
 * @hp 4
 * @energy 2
 */
define character {
  id 3703 as DendroSamachurl;
  tags dendro, monster, hilichurl;
  health 4;
  energy 2;
  skills DendroSamachurlStaffStrike, VenomousVines, DendroField;
};
