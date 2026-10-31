// Arithmetic Enhancer Mek - Pneuma (3010): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages ("Ram (Arithmetic Enhancer Mek - Pneuma)": 2 Physical,
// Energy Boost: Pneuma: 2 Physical, Energy Burst: Pneuma: 3 Physical).
// Piovium's Arkhe tags are swapped relative to the English names: the tag "ousia" means
// English "Pneuma" and the tag "pneuma" means English "Ousia" (see core/src/base/character.ts).
// So this character carries the tag "ousia" and is deactivated by attackers tagged "pneuma".
// Energy Boost: Pneuma 130101 shows a 1-round counter in the game data, so it lasts 1 round.
// The game has no status id for "Deactivated"; 130102 is an unused id in this character's range.
import { $, DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 130102
 * @name Deactivated
 * @description
 * Character cannot use Skills. (Lasts until the end of this Round)
 */
define status {
  id 130102 as ArithmeticMekPneumaDeactivated;
  oneDuration;
  tags disableSkill;
};

/**
 * @id 130101
 * @name Energy Boost: Pneuma
 * @description
 * When the Frame Repair: Pneuma attached to this character triggers, it will heal +1 HP.
 */
define status {
  id 130101 as EnergyBoostPneuma;
  oneDuration;
};

/**
 * @id 30101
 * @name Ram
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 30101 as ArithmeticMekPneumaRam;
  skillType normal;
  cost DiceType.Void, 3;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 30102
 * @name Energy Boost: Pneuma
 * @description
 * Deals 2 Physical DMG. This character gains Energy Boost: Pneuma.
 */
define skill {
  id 30102 as EnergyBoostPneumaSkill;
  skillType elemental;
  cost DiceType.Aligned, 3;
  :damage(DamageType.Physical, 2);
  :characterStatus(EnergyBoostPneuma);
};

/**
 * @id 30103
 * @name Energy Burst: Pneuma
 * @description
 * Deals 3 Physical DMG. This character gains Energy Boost: Pneuma.
 */
define skill {
  id 30103 as EnergyBurstPneuma;
  skillType burst;
  cost DiceType.Aligned, 3;
  cost DiceType.Energy, 1;
  :damage(DamageType.Physical, 3);
  :characterStatus(EnergyBoostPneuma);
};

/**
 * @id 30104
 * @name Frame Repair: Pneuma
 * @description
 * After your character uses a Skill: Heal your most injured character for 1 HP.
 */
define skill {
  id 30104 as FrameRepairPneuma;
  skillType passive {
    on useSkill {
      listenTo samePlayer;
      if (:self.hasStatus(EnergyBoostPneuma)) {
        :heal(2, $.my.character.orderBy("health", "-", "maxHealth").limit(1));
      } else {
        :heal(1, $.my.character.orderBy("health", "-", "maxHealth").limit(1));
      }
    };
  };
};

/**
 * @id 30105
 * @name Arkhe: Pneuma
 * @description
 * After being damaged by characters with "Arkhe: Ousia," this character will become Deactivated,
 * and cannot use skills until this Round ends.
 */
define skill {
  id 30105 as ArithmeticMekArkhePneuma;
  skillType passive {
    on damaged {
      when :(
        :e.source.definition.type === "character" &&
          :e.source.definition.tags.includes("pneuma")
      );
      :characterStatus(ArithmeticMekPneumaDeactivated, :self);
    };
  };
};

/**
 * @id 3010
 * @name Arithmetic Enhancer Mek - Pneuma
 * @hp 4
 * @energy 1
 */
define character {
  id 3010 as ArithmeticEnhancerMekPneuma;
  tags monster, ousia;
  health 4;
  energy 1;
  skills ArithmeticMekPneumaRam, EnergyBoostPneumaSkill, EnergyBurstPneuma, FrameRepairPneuma, ArithmeticMekArkhePneuma;
};
