// Annihilation Specialist Mek - Pneuma (3411): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages (Piercing Blow: 2 Physical, Lightning Raid: 3 Electro,
// Leaping Thunder: 5 Electro).
// Area Alert Mek - Pneuma 134111: the game data shows a usage of 1 plus a counter, so it fires once,
// dealing 1 Hydro DMG to the opposing active character after the opponent's 3rd Action Card, then leaves.
// "When taking DMG from a character with Arkhe: Ousia" is read as any character on this side taking such DMG.
// Piovium's Arkhe tags are swapped relative to the English names: the tag "ousia" means English
// "Pneuma", so this character carries "ousia" and reacts to attackers tagged "pneuma" (English Ousia).
// The game has no status id for "Deactivated"; 134112 is an unused id in this character's range.
import { $, DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 134112
 * @name Deactivated
 * @description
 * Character cannot use Skills. (Lasts until the end of this Round)
 */
define status {
  id 134112 as AnnihilationMekDeactivated;
  oneDuration;
  tags disableSkill;
};

/**
 * @id 134111
 * @name Area Alert Mek - Pneuma
 * @description
 * After your opponent plays 3 Action Cards, deal 1 Hydro DMG.
 * When taking DMG from a character with Arkhe: Ousia: Cancel this effect.
 */
define combatStatus {
  id 134111 as AreaAlertMekPneuma;
  variable opponentCardsPlayed, 0;
  on playCard {
    listenTo all;
    when :( :e.who !== :self.who );
    :addVariable("opponentCardsPlayed", 1);
    if (:getVariable("opponentCardsPlayed") >= 3) {
      :damage(DamageType.Hydro, 1, $.opp.active);
      :dispose();
    }
  };
  on damaged {
    listenTo samePlayer;
    when :(
      :e.source.definition.type === "character" &&
        :e.source.definition.tags.includes("pneuma")
    );
    :dispose();
  };
};

/**
 * @id 34111
 * @name Piercing Blow
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 34111 as PiercingBlow;
  skillType normal;
  cost DiceType.Electro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 34112
 * @name Lightning Raid
 * @description
 * Deals 3 Electro DMG.
 */
define skill {
  id 34112 as LightningRaid;
  skillType elemental;
  cost DiceType.Electro, 3;
  :damage(DamageType.Electro, 3);
};

/**
 * @id 34113
 * @name Leaping Thunder
 * @description
 * Deals 5 Electro DMG.
 */
define skill {
  id 34113 as LeapingThunder;
  skillType burst;
  cost DiceType.Electro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Electro, 5);
};

/**
 * @id 34114
 * @name Support Unit: Pneuma
 * @description
 * (Passive) When battle begins, create 1 initial Area Alert Mek - Pneuma.
 */
define skill {
  id 34114 as SupportUnitPneuma;
  skillType passive {
    on battleBegin {
      :combatStatus(AreaAlertMekPneuma);
    };
  };
};

/**
 * @id 34115
 * @name Arkhe: Pneuma
 * @description
 * After being damaged by characters with "Arkhe: Ousia," this character will become Deactivated,
 * and cannot use skills until this Round ends.
 */
define skill {
  id 34115 as AnnihilationMekArkhePneuma;
  skillType passive {
    on damaged {
      when :(
        :e.source.definition.type === "character" &&
          :e.source.definition.tags.includes("pneuma")
      );
      :characterStatus(AnnihilationMekDeactivated, :self);
    };
  };
};

/**
 * @id 3411
 * @name Annihilation Specialist Mek - Pneuma
 * @hp 10
 * @energy 2
 */
define character {
  id 3411 as AnnihilationSpecialistMekPneuma;
  tags electro, monster, ousia;
  health 10;
  energy 2;
  skills PiercingBlow, LightningRaid, LeapingThunder, SupportUnitPneuma, AnnihilationMekArkhePneuma;
};
