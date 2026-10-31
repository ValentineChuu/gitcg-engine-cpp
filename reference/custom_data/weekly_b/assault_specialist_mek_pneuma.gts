// Assault Specialist Mek - Pneuma (3307): NPC-only monster written from the game description.
// Damage values: Dimbreath declared values (Multi-Smash: 1 Pyro) and fandom wiki skill pages
// (Up-Close Pummelling: 2 Physical, Sprinting Strike: 5 Pyro, Tracking Truncheon: 2 Pyro).
// The game has no status id for "Deactivated"; 133072 is an unused id in this character's range.
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 133072
 * @name Deactivated
 * @description
 * Character cannot use Skills. (Lasts until the end of this Round)
 */
define status {
  id 133072 as AssaultMekDeactivated;
  oneDuration;
  tags disableSkill;
};

/**
 * @id 33075
 * @name Tracking Truncheon
 * @description
 * (Prepare for 1 turn)
 * Deals 2 Pyro DMG.
 */
define skill {
  id 33075 as TrackingTruncheon;
  skillType elemental;
  prepared;
  :damage(DamageType.Pyro, 2);
};

/**
 * @id 133071
 * @name Tailing Truncheon
 * @description
 * The next time this character acts, they will immediately use the Skill Tracking Truncheon.
 */
define status {
  id 133071 as TailingTruncheon;
  prepare TrackingTruncheon;
};

/**
 * @id 33071
 * @name Up-Close Pummelling
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 33071 as UpClosePummelling;
  skillType normal;
  cost DiceType.Pyro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 33072
 * @name Multi-Smash
 * @description
 * Deals 1 Pyro DMG and prepares Tracking Truncheon.
 */
define skill {
  id 33072 as MultiSmash;
  skillType elemental;
  cost DiceType.Pyro, 3;
  :damage(DamageType.Pyro, 1);
  :characterStatus(TailingTruncheon);
};

/**
 * @id 33073
 * @name Sprinting Strike
 * @description
 * Deals 5 Pyro DMG.
 */
define skill {
  id 33073 as SprintingStrike;
  skillType burst;
  cost DiceType.Pyro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Pyro, 5);
};

/**
 * @id 33074
 * @name Arkhe: Pneuma
 * @description
 * After being damaged by characters with "Arkhe: Ousia," this character will become Deactivated,
 * and cannot use skills until this Round ends.
 */
define skill {
  id 33074 as AssaultMekArkhePneuma;
  skillType passive {
    on damaged {
      when :(
        :e.source.definition.type === "character" &&
          :e.source.definition.tags.includes("pneuma") /* Piovium tag "pneuma" = English Arkhe: Ousia */
      );
      :characterStatus(AssaultMekDeactivated, :self);
    };
  };
};

/**
 * @id 3307
 * @name Assault Specialist Mek - Pneuma
 * @hp 10
 * @energy 2
 */
define character {
  id 3307 as AssaultSpecialistMekPneuma;
  tags pyro, monster, ousia /* English Arkhe: Pneuma */;
  health 10;
  energy 2;
  skills UpClosePummelling, MultiSmash, SprintingStrike, AssaultMekArkhePneuma, TrackingTruncheon;
};
