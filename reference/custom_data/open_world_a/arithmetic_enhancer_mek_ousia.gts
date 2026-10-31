// Arithmetic Enhancer Mek - Ousia (3009): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages ("Ram (Arithmetic Enhancer Mek - Ousia)": 2 Physical,
// Energy Boost: Ousia: 2 Physical, Energy Burst: Ousia: 3 Physical).
// Piovium's Arkhe tags are swapped relative to the English names: the tag "pneuma" means
// English "Ousia" and the tag "ousia" means English "Pneuma" (see core/src/base/character.ts).
// So this character carries the tag "pneuma" and is deactivated by attackers tagged "ousia".
// Energy Boost: Ousia 130091 shows a 1-round counter in the game data, so it lasts 1 round.
// The game has no status id for "Deactivated"; 130092 is an unused id in this character's range.
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 130092
 * @name Deactivated
 * @description
 * Character cannot use Skills. (Lasts until the end of this Round)
 */
define status {
  id 130092 as ArithmeticMekOusiaDeactivated;
  oneDuration;
  tags disableSkill;
};

/**
 * @id 130091
 * @name Energy Boost: Ousia
 * @description
 * When the Frame Enhancement: Ousia attached to this character takes effect, all your characters will deal
 * +1 additional DMG.
 */
define status {
  id 130091 as EnergyBoostOusia;
  oneDuration;
};

/**
 * @id 30091
 * @name Ram
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 30091 as ArithmeticMekOusiaRam;
  skillType normal;
  cost DiceType.Void, 3;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 30092
 * @name Energy Boost: Ousia
 * @description
 * Deals 2 Physical DMG. This character gains Energy Boost: Ousia.
 */
define skill {
  id 30092 as EnergyBoostOusiaSkill;
  skillType elemental;
  cost DiceType.Aligned, 3;
  :damage(DamageType.Physical, 2);
  :characterStatus(EnergyBoostOusia);
};

/**
 * @id 30093
 * @name Energy Burst: Ousia
 * @description
 * Deals 3 Physical DMG. This character gains Energy Boost: Ousia.
 */
define skill {
  id 30093 as EnergyBurstOusia;
  skillType burst;
  cost DiceType.Aligned, 3;
  cost DiceType.Energy, 1;
  :damage(DamageType.Physical, 3);
  :characterStatus(EnergyBoostOusia);
};

/**
 * @id 30094
 * @name Frame Enhancement: Ousia
 * @description
 * Your characters deal +1 DMG.
 */
define skill {
  id 30094 as FrameEnhancementOusia;
  skillType passive {
    on increaseSkillDamage {
      listenTo samePlayer;
      if (:self.hasStatus(EnergyBoostOusia)) {
        :e.increaseDamage(2);
      } else {
        :e.increaseDamage(1);
      }
    };
  };
};

/**
 * @id 30095
 * @name Arkhe: Ousia
 * @description
 * After being damaged by characters with "Arkhe: Pneuma," this character will become Deactivated,
 * and cannot use skills until this Round ends.
 */
define skill {
  id 30095 as ArithmeticMekArkheOusia;
  skillType passive {
    on damaged {
      when :(
        :e.source.definition.type === "character" &&
          :e.source.definition.tags.includes("ousia")
      );
      :characterStatus(ArithmeticMekOusiaDeactivated, :self);
    };
  };
};

/**
 * @id 3009
 * @name Arithmetic Enhancer Mek - Ousia
 * @hp 4
 * @energy 1
 */
define character {
  id 3009 as ArithmeticEnhancerMekOusia;
  tags monster, pneuma;
  health 4;
  energy 1;
  skills ArithmeticMekOusiaRam, EnergyBoostOusiaSkill, EnergyBurstOusia, FrameEnhancementOusia, ArithmeticMekArkheOusia;
};
