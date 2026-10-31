// Eremite Sunfrost (3106): Tavern Challenge companion. Damage values from the fandom wiki skill pages.
// Stalwart Onslaught prepares All-Out Strike over two turns: Charging Stance (31066) is the intermediate
// prepared step, as the game's statuses 131062 ("use All-Out Strike the second time they act") and
// 131063 ("the next time they act") describe.
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 131061
 * @name Spirit of Omen's Awakening: Cryo
 * Character deals +1 DMG and converts Physical DMG to Cryo DMG. This character takes +2 Cryo DMG.
 * Duration (Rounds): 2
 */
define status {
  id 131061 as SpiritOfOmensAwakeningCryo;
  duration 2;
  on modifySkillDamageType {
    when :( :e.type === DamageType.Physical );
    :e.changeDamageType(DamageType.Cryo);
  };
  on increaseSkillDamage {
    :e.increaseDamage(1);
  };
  on increaseDamaged {
    when :( :e.type === DamageType.Cryo );
    :e.increaseDamage(2);
  };
};

/**
 * @id 31065
 * @name Stalwart Onslaught: All-Out Strike
 * (Prepare for 2 turns) Deals 6 Physical DMG.
 */
define skill {
  id 31065 as StalwartOnslaughtAllOutStrike;
  skillType burst;
  prepared;
  :damage(DamageType.Physical, 6);
};

/**
 * @id 131063
 * @name Charged Attack: Prep Complete
 * The next time this character acts, they will immediately use All-Out Strike.
 */
define status {
  id 131063 as ChargedAttackPrepComplete;
  prepare StalwartOnslaughtAllOutStrike;
};

/**
 * @id 31066
 * @name Stalwart Onslaught: Charging Stance
 * (Prepared for 1 Turn) The next time this character acts, they will use All-Out Strike.
 */
define skill {
  id 31066 as StalwartOnslaughtChargingStance;
  skillType burst;
  prepared;
  :characterStatus(ChargedAttackPrepComplete, :self);
};

/**
 * @id 131062
 * @name Charged Attack: In Position
 * This character will immediately use All-Out Strike the second time they act after preparation starts.
 */
define status {
  id 131062 as ChargedAttackInPosition;
  prepare StalwartOnslaughtChargingStance;
};

/**
 * @id 31061
 * @name Double Hack
 * Deals 2 Physical DMG.
 */
define skill {
  id 31061 as DoubleHack;
  skillType normal;
  cost DiceType.Cryo, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 31062
 * @name Dual-Blade Chop
 * Deals 3 Physical DMG.
 */
define skill {
  id 31062 as DualBladeChop;
  skillType elemental;
  cost DiceType.Cryo, 3;
  :damage(DamageType.Physical, 3);
};

/**
 * @id 31063
 * @name Stalwart Onslaught
 * This character prepares All-Out Strike.
 */
define skill {
  id 31063 as StalwartOnslaught;
  skillType burst;
  cost DiceType.Cryo, 3;
  cost DiceType.Energy, 2;
  :characterStatus(ChargedAttackInPosition, :self);
};

/**
 * @id 31064
 * @name Ominous Spirit Unleashed (Cryo)
 * (Passive) After this character takes DMG: if they have no more than 7 HP, they gain 1 Energy and
 * Spirit of Omen's Awakening: Cryo is attached. (Once per Match)
 */
define skill {
  id 31064 as OminousSpiritUnleashedCryo;
  skillType passive {
    on damaged {
      when :( :self.health <= 7 );
      usage 1 {
        name "ominousSpiritUsage";
      };
      :gainEnergy(1, :self);
      :characterStatus(SpiritOfOmensAwakeningCryo, :self);
    };
  };
};

define character {
  id 3106 as EremiteSunfrost;
  tags cryo, eremite;
  health 8;
  energy 2;
  skills DoubleHack,
    DualBladeChop,
    StalwartOnslaught,
    OminousSpiritUnleashedCryo,
    StalwartOnslaughtAllOutStrike,
    StalwartOnslaughtChargingStance;
};
