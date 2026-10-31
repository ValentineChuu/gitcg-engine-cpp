// Eremite Stone Enchanter (3606): Tavern Challenge companion. Damage values from the fandom wiki skill pages.
// The Stone Drake's protection lives in Stone Drake: Guardian Stance (136052), which the passive 36065 attaches
// at battle begin; its once-per-round use refreshes each round while the Drake is in play.
import { $, DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 136051
 * @name Spirit of Omen: Stone Drake
 * End Phase: Deals 1 Geo DMG. Usage(s): 3
 * While in play, reduces DMG taken by Eremite Stone Enchanter by 2 (see 136052).
 */
define summon {
  id 136051 as SpiritOfOmenStoneDrake;
  hint DamageType.Geo, 1;
  on endPhase {
    usage 3;
    :damage(DamageType.Geo, 1);
  };
};

/**
 * @id 136052
 * @name Stone Drake: Guardian Stance
 * When Spirit of Omen: Stone Drake is in play and this character takes DMG: reduce it by 2, consuming 1 of the
 * Drake's Usages (1 more when the DMG is Geo). (Once per Round. Usage consumption required)
 */
define status {
  id 136052 as StoneDrakeGuardianStance;
  tags barrier;
  on decreaseDamaged {
    when :( !!:query($.my.summon.def(SpiritOfOmenStoneDrake)) );
    usage perRound, 1;
    const drake = :query($.my.summon.def(SpiritOfOmenStoneDrake))!;
    :e.decreaseDamage(2);
    :consumeUsage(:e.type === DamageType.Geo ? 2 : 1, drake);
  };
};

/**
 * @id 36061
 * @name Triple Geosmash
 * Deals 2 Physical DMG.
 */
define skill {
  id 36061 as TripleGeosmash;
  skillType normal;
  cost DiceType.Geo, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 36062
 * @name Splitrock Slam
 * Deals 3 Geo DMG.
 */
define skill {
  id 36062 as SplitrockSlam;
  skillType elemental;
  cost DiceType.Geo, 3;
  :damage(DamageType.Geo, 3);
};

/**
 * @id 36063
 * @name Ominous Spirit Unleashed (Geo)
 * Deals 3 Geo DMG, summons 1 Spirit of Omen: Stone Drake.
 */
define skill {
  id 36063 as OminousSpiritUnleashedGeo;
  skillType burst;
  cost DiceType.Geo, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Geo, 3);
  :summon(SpiritOfOmenStoneDrake);
};

/**
 * @id 36064
 * @name Spirit of Omen's Power
 * (Passive) After this character takes DMG: if they have no more than 7 HP, they gain 1 Energy. (Once per Round)
 */
define skill {
  id 36064 as SpiritOfOmensPowerGeo;
  skillType passive {
    on damaged {
      when :( :self.health <= 7 && :self.energy < :self.maxEnergy );
      usage perRound, 1 {
        name "usagePerRound1";
      };
      :gainEnergy(1, :self);
    };
  };
};

/**
 * @id 36065
 * @name Stone Drake: Guardian Stance
 * (Passive) When the battle begins, this character gains Stone Drake: Guardian Stance (136052).
 */
define skill {
  id 36065 as StoneDrakeGuardianStanceSkill;
  skillType passive {
    on battleBegin {
      :characterStatus(StoneDrakeGuardianStance);
    };
  };
};

define character {
  id 3606 as EremiteStoneEnchanter;
  tags geo, eremite;
  health 10;
  energy 2;
  skills TripleGeosmash,
    SplitrockSlam,
    OminousSpiritUnleashedGeo,
    SpiritOfOmensPowerGeo,
    StoneDrakeGuardianStanceSkill;
};
