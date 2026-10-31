// Suppression Specialist Mek - Pneuma (3704): NPC-only monster written from the game description.
// Damage values: Dimbreath declared values (Launch Grenade: 2 Dendro) and fandom wiki skill pages
// (Precision Shot: 1 Dendro, Wildgrass Ball: 3 Dendro, Delayed Fuse Munitions: 2 Dendro + 1 Piercing).
// The game has no status id for "Deactivated"; 137042 is an unused id in this character's range.
import { $, DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 137042
 * @name Deactivated
 * @description
 * Character cannot use Skills. (Lasts until the end of this Round)
 */
define status {
  id 137042 as SuppressionMekDeactivated;
  oneDuration;
  tags disableSkill;
};

/**
 * @id 137041
 * @name Delayed Fuse Munitions
 * @description
 * End Phase: Deal 2 Dendro DMG, deal 1 Piercing DMG to all opposing characters on standby.
 * Usage(s): 1
 */
define summon {
  id 137041 as DelayedFuseMunitions;
  hint DamageType.Dendro, 2;
  on endPhase {
    usage 1;
    :damage(DamageType.Piercing, 1, $.opp.standby);
    :damage(DamageType.Dendro, 2);
  };
};

/**
 * @id 37041
 * @name Precision Shot
 * @description
 * Deals 1 Dendro DMG.
 */
define skill {
  id 37041 as PrecisionShot;
  skillType normal;
  cost DiceType.Dendro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Dendro, 1);
};

/**
 * @id 37042
 * @name Wildgrass Ball
 * @description
 * Deals 3 Dendro DMG.
 */
define skill {
  id 37042 as WildgrassBall;
  skillType elemental;
  cost DiceType.Dendro, 3;
  :damage(DamageType.Dendro, 3);
};

/**
 * @id 37043
 * @name Launch Grenade
 * @description
 * Deals 2 Dendro DMG, summons 1 Delayed Fuse Munitions.
 */
define skill {
  id 37043 as LaunchGrenade;
  skillType burst;
  cost DiceType.Dendro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Dendro, 2);
  :summon(DelayedFuseMunitions);
};

/**
 * @id 37044
 * @name Arkhe: Pneuma
 * @description
 * After being damaged by characters with "Arkhe: Ousia," this character will become Deactivated,
 * and cannot use skills until this Round ends.
 */
define skill {
  id 37044 as SuppressionMekArkhePneuma;
  skillType passive {
    on damaged {
      when :(
        :e.source.definition.type === "character" &&
          :e.source.definition.tags.includes("pneuma") /* Piovium tag "pneuma" = English Arkhe: Ousia */
      );
      :characterStatus(SuppressionMekDeactivated, :self);
    };
  };
};

/**
 * @id 3704
 * @name Suppression Specialist Mek - Pneuma
 * @hp 10
 * @energy 2
 */
define character {
  id 3704 as SuppressionSpecialistMekPneuma;
  tags dendro, monster, ousia /* English Arkhe: Pneuma */;
  health 10;
  energy 2;
  skills PrecisionShot, WildgrassBall, LaunchGrenade, SuppressionMekArkhePneuma;
};
