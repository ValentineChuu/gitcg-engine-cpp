// Assault Specialist Mek - Ousia (3408): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages (Rapid Melee: 2 Physical, Shield Sweep: 2 Electro + 2 Piercing,
// Shield Slice: 3 Electro).
// Shield Block 134081 is both the prepare status for Shield Slice and the 2-point shield: Piovium
// disposes a prepare status when the prepared skill fires, so the shield lasts only while preparing.
// Piovium's Arkhe tags are swapped relative to the English names: the tag "pneuma" means
// English "Ousia", so this character carries "pneuma" and is deactivated by attackers tagged "ousia".
// The game has no status id for "Deactivated"; 134082 is an unused id in this character's range.
import { $, DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 134082
 * @name Deactivated
 * @description
 * Character cannot use Skills. (Lasts until the end of this Round)
 */
define status {
  id 134082 as AssaultMekOusiaDeactivated;
  oneDuration;
  tags disableSkill;
};

/**
 * @id 34085
 * @name Shield Slice
 * @description
 * (Prepare for 1 turn)
 * Deals 3 Electro DMG.
 */
define skill {
  id 34085 as ShieldSlice;
  skillType elemental;
  prepared;
  :damage(DamageType.Electro, 3);
};

/**
 * @id 134081
 * @name Shield Block
 * @description
 * The next time this character acts, they will immediately use the Skill Shield Slice.
 * While preparing Shield Slice: Grant 2 Shield points to this character.
 */
define status {
  id 134081 as ShieldBlock;
  shield 2;
  prepare ShieldSlice;
};

/**
 * @id 34081
 * @name Rapid Melee
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 34081 as RapidMelee;
  skillType normal;
  cost DiceType.Electro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 34082
 * @name Shield Riposte
 * @description
 * This character gains a Shield Block. Prepare Skill: Shield Slice.
 */
define skill {
  id 34082 as ShieldRiposte;
  skillType elemental;
  cost DiceType.Electro, 3;
  :characterStatus(ShieldBlock);
};

/**
 * @id 34083
 * @name Shield Sweep
 * @description
 * Deals 2 Electro DMG, deals 2 Piercing DMG to all opposing characters on standby.
 */
define skill {
  id 34083 as ShieldSweep;
  skillType burst;
  cost DiceType.Electro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Piercing, 2, $.opp.standby);
  :damage(DamageType.Electro, 2);
};

/**
 * @id 34084
 * @name Arkhe: Ousia
 * @description
 * After being damaged by characters with "Arkhe: Pneuma," this character will become Deactivated,
 * and cannot use skills until this Round ends.
 */
define skill {
  id 34084 as AssaultMekArkheOusia;
  skillType passive {
    on damaged {
      when :(
        :e.source.definition.type === "character" &&
          :e.source.definition.tags.includes("ousia")
      );
      :characterStatus(AssaultMekOusiaDeactivated, :self);
    };
  };
};

/**
 * @id 3408
 * @name Assault Specialist Mek - Ousia
 * @hp 10
 * @energy 2
 */
define character {
  id 3408 as AssaultSpecialistMekOusia;
  tags electro, monster, pneuma;
  health 10;
  energy 2;
  skills RapidMelee, ShieldRiposte, ShieldSweep, AssaultMekArkheOusia, ShieldSlice;
};
