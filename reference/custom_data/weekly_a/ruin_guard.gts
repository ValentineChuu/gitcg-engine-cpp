// Ruin Guard (3004). Damage values from the fandom wiki skill pages.
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 130041
 * @name Seeking Missile
 * @description
 * After a character switch takes place in this play area: Deal 2 Physical DMG to the character activated.
 * Usage(s): 2
 */
define combatStatus {
  id 130041 as SeekingMissile;
  on switchActive {
    usage 2;
    :damage(DamageType.Physical, 2, :e.switchInfo.to);
  };
};

/**
 * @id 30046
 * @name Hyperspeed Whirling Assault
 * @description
 * (Prepare for 1 turn)
 * Deals 1 Physical DMG. This character also gains 1 bonus Energy.
 */
define skill {
  id 30046 as HyperspeedWhirlingAssault;
  skillType elemental;
  prepared;
  :damage(DamageType.Physical, 1);
  :gainEnergy(1, :self);
};

/**
 * @id 130043
 * @name Hyperspeed Whirling Assault: Preparatory Posture
 * @description
 * The next time this character acts, they will immediately use the Skill Hyperspeed Whirling Assault.
 */
define status {
  id 130043 as HyperspeedWhirlingAssaultPosture;
  prepare HyperspeedWhirlingAssault;
};

/**
 * @id 30045
 * @name High-Frequency Whirling Assault
 * @description
 * (Prepare for 1 turn)
 * Deals 1 Physical DMG. Then, perform "Prepare Skill" for Hyperspeed Whirling Assault.
 */
define skill {
  id 30045 as HighFrequencyWhirlingAssault;
  skillType elemental;
  prepared;
  :damage(DamageType.Physical, 1);
};

/**
 * @id 130042
 * @name High-Frequency Whirling Assault: Preparatory Posture
 * @description
 * The next time this character acts, they will immediately use the Skill High-Frequency Whirling Assault.
 */
define status {
  id 130042 as HighFrequencyWhirlingAssaultPosture;
  prepare HighFrequencyWhirlingAssault {
    nextStatus HyperspeedWhirlingAssaultPosture;
  };
};

/**
 * @id 30041
 * @name Stern Blow
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 30041 as SternBlow;
  skillType normal;
  cost DiceType.Void, 3;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 30042
 * @name Spiral Close-In
 * @description
 * Deals 1 Physical DMG and then separately performs "Prepare Skill" for High-Frequency Whirling Assault
 * and Hyperspeed Whirling Assault.
 */
define skill {
  id 30042 as SpiralCloseIn;
  skillType elemental;
  cost DiceType.Aligned, 3;
  :damage(DamageType.Physical, 1);
  :characterStatus(HighFrequencyWhirlingAssaultPosture);
};

/**
 * @id 30043
 * @name Suppressive Volley
 * @description
 * Deals 4 Physical DMG, creates 1 Seeking Missile in the opponent's play area.
 */
define skill {
  id 30043 as SuppressiveVolley;
  skillType burst;
  cost DiceType.Aligned, 4;
  cost DiceType.Energy, 2;
  :damage(DamageType.Physical, 4);
  :combatStatus(SeekingMissile, "opp");
};

/**
 * @id 30044
 * @name Standard Armor
 * @description
 * (Passive) The Physical DMG this character receives is halved (Rounded up)
 */
define skill {
  id 30044 as RuinGuardStandardArmor;
  skillType passive {
    on multiplyDamaged {
      when :( :e.type === DamageType.Physical );
      :e.divideDamage(2);
    };
  };
};

define character {
  id 3004 as RuinGuard;
  tags monster;
  health 8;
  energy 2;
  skills SternBlow, SpiralCloseIn, SuppressiveVolley, RuinGuardStandardArmor,
    HighFrequencyWhirlingAssault, HyperspeedWhirlingAssault;
};
