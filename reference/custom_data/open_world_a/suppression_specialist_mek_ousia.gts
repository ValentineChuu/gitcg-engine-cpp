// Suppression Specialist Mek - Ousia (3308): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages (Short-Range Snap Fire: 1 Pyro, Spread Fire: 1 Pyro + 1 Piercing,
// Combined Fire: 2 Pyro, Combined Fire: Light Barrage: 2 Pyro, Combined Fire: Piercing Salvo: 2 Pyro).
// The prepare statuses use the game's own ids 133081 / 133082, whose game data says they make the
// character use 33085 / 33086. Their English names in the text map are "Long-Range Fire: Calibrated Shot"
// and "Burst Fire: Calibrated Shot".
// Combined Fire "separately prepares" Light Barrage and then Piercing Salvo: modelled as a chain,
// Combined Fire -> Light Barrage (next turn) -> Piercing Salvo (turn after).
// Piovium's Arkhe tags are swapped relative to the English names: the tag "pneuma" means
// English "Ousia", so this character carries "pneuma" and is deactivated by attackers tagged "ousia".
// The game has no status id for "Deactivated"; 133083 is an unused id in this character's range.
import { $, DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 133083
 * @name Deactivated
 * @description
 * Character cannot use Skills. (Lasts until the end of this Round)
 */
define status {
  id 133083 as SuppressionMekOusiaDeactivated;
  oneDuration;
  tags disableSkill;
};

/**
 * @id 33086
 * @name Combined Fire: Piercing Salvo
 * @description
 * (Prepare for 1 turn)
 * Deals 2 Pyro DMG.
 */
define skill {
  id 33086 as CombinedFirePiercingSalvo;
  skillType burst;
  prepared;
  :damage(DamageType.Pyro, 2);
};

/**
 * @id 33085
 * @name Combined Fire: Light Barrage
 * @description
 * (Prepare for 1 turn)
 * Deals 2 Pyro DMG. Then, perform "Prepare Skill" for Combined Fire: Piercing Salvo.
 */
define skill {
  id 33085 as CombinedFireLightBarrage;
  skillType burst;
  prepared;
  :damage(DamageType.Pyro, 2);
};

/**
 * @id 133082
 * @name Burst Fire: Calibrated Shot
 * @description
 * The next time this character acts, they will immediately use the Skill Combined Fire: Piercing Salvo.
 */
define status {
  id 133082 as PiercingSalvoPreparation;
  prepare CombinedFirePiercingSalvo;
};

/**
 * @id 133081
 * @name Long-Range Fire: Calibrated Shot
 * @description
 * The next time this character acts, they will immediately use the Skill Combined Fire: Light Barrage.
 */
define status {
  id 133081 as LightBarragePreparation;
  prepare CombinedFireLightBarrage {
    nextStatus PiercingSalvoPreparation;
  };
};

/**
 * @id 33081
 * @name Short-Range Snap Fire
 * @description
 * Deals 1 Pyro DMG.
 */
define skill {
  id 33081 as ShortRangeSnapFire;
  skillType normal;
  cost DiceType.Pyro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Pyro, 1);
};

/**
 * @id 33082
 * @name Spread Fire
 * @description
 * Deals 1 Pyro DMG, deals 1 Piercing DMG to all opposing characters on standby.
 */
define skill {
  id 33082 as SpreadFire;
  skillType elemental;
  cost DiceType.Pyro, 3;
  :damage(DamageType.Piercing, 1, $.opp.standby);
  :damage(DamageType.Pyro, 1);
};

/**
 * @id 33083
 * @name Combined Fire
 * @description
 * Deals 2 Pyro DMG and then separately performs "Prepare Skill" for Combined Fire: Light Barrage and
 * Combined Fire: Piercing Salvo.
 */
define skill {
  id 33083 as CombinedFire;
  skillType burst;
  cost DiceType.Pyro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Pyro, 2);
  :characterStatus(LightBarragePreparation);
};

/**
 * @id 33084
 * @name Arkhe: Ousia
 * @description
 * After being damaged by characters with "Arkhe: Pneuma," this character will become Deactivated,
 * and cannot use skills until this Round ends.
 */
define skill {
  id 33084 as SuppressionMekArkheOusia;
  skillType passive {
    on damaged {
      when :(
        :e.source.definition.type === "character" &&
          :e.source.definition.tags.includes("ousia")
      );
      :characterStatus(SuppressionMekOusiaDeactivated, :self);
    };
  };
};

/**
 * @id 3308
 * @name Suppression Specialist Mek - Ousia
 * @hp 10
 * @energy 2
 */
define character {
  id 3308 as SuppressionSpecialistMekOusia;
  tags pyro, monster, pneuma;
  health 10;
  energy 2;
  skills ShortRangeSnapFire, SpreadFire, CombinedFire, SuppressionMekArkheOusia, CombinedFireLightBarrage, CombinedFirePiercingSalvo;
};
