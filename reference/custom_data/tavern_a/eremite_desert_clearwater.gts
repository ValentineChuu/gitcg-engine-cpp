// Eremite Desert Clearwater (3206): Tavern Challenge companion. Damage values from the fandom wiki skill pages.
// Whirling Lashes prepares First Strike, which deals its DMG and then prepares Fixed Pattern.
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 132061
 * @name Spirit of Omen's Awakening: Hydro
 * Character deals +1 DMG and converts Physical DMG to Hydro DMG. This character takes +2 Hydro DMG.
 * Duration (Rounds): 2
 */
define status {
  id 132061 as SpiritOfOmensAwakeningHydro;
  duration 2;
  on modifySkillDamageType {
    when :( :e.type === DamageType.Physical );
    :e.changeDamageType(DamageType.Hydro);
  };
  on increaseSkillDamage {
    :e.increaseDamage(1);
  };
  on increaseDamaged {
    when :( :e.type === DamageType.Hydro );
    :e.increaseDamage(2);
  };
};

/**
 * @id 32066
 * @name Whirling Lashes: Fixed Pattern
 * (Prepare for 1 turn) Deals 3 Physical DMG.
 */
define skill {
  id 32066 as WhirlingLashesFixedPattern;
  skillType burst;
  prepared;
  :damage(DamageType.Physical, 3);
};

/**
 * @id 132063
 * @name Refractory Whirling Dance: Prep Complete
 * The next time this character acts, they will immediately use Fixed Pattern.
 */
define status {
  id 132063 as RefractoryWhirlingDancePrepComplete;
  prepare WhirlingLashesFixedPattern;
};

/**
 * @id 32065
 * @name Whirling Lashes: First Strike
 * (Prepare for 1 turn) Deals 2 Physical DMG. Then, prepares Fixed Pattern.
 */
define skill {
  id 32065 as WhirlingLashesFirstStrike;
  skillType burst;
  prepared;
  :damage(DamageType.Physical, 2);
  :characterStatus(RefractoryWhirlingDancePrepComplete, :self);
};

/**
 * @id 132062
 * @name Refractory Whirling Hew: Prep Complete
 * The next time this character acts, they will immediately use First Strike.
 */
define status {
  id 132062 as RefractoryWhirlingHewPrepComplete;
  prepare WhirlingLashesFirstStrike;
};

/**
 * @id 32061
 * @name Agile Thrust
 * Deals 2 Physical DMG.
 */
define skill {
  id 32061 as AgileThrust;
  skillType normal;
  cost DiceType.Hydro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 32062
 * @name Blitz Strike
 * Deals 3 Physical DMG.
 */
define skill {
  id 32062 as BlitzStrike;
  skillType elemental;
  cost DiceType.Hydro, 3;
  :damage(DamageType.Physical, 3);
};

/**
 * @id 32063
 * @name Whirling Lashes
 * This character prepares First Strike and Fixed Pattern separately.
 */
define skill {
  id 32063 as WhirlingLashes;
  skillType burst;
  cost DiceType.Hydro, 3;
  cost DiceType.Energy, 2;
  :characterStatus(RefractoryWhirlingHewPrepComplete, :self);
};

/**
 * @id 32064
 * @name Ominous Spirit Unleashed (Hydro)
 * (Passive) After this character takes DMG: if they have no more than 7 HP, they gain 1 Energy and
 * Spirit of Omen's Awakening: Hydro is attached. (Once per Match)
 */
define skill {
  id 32064 as OminousSpiritUnleashedHydro;
  skillType passive {
    on damaged {
      when :( :self.health <= 7 );
      usage 1 {
        name "ominousSpiritUsage";
      };
      :gainEnergy(1, :self);
      :characterStatus(SpiritOfOmensAwakeningHydro, :self);
    };
  };
};

define character {
  id 3206 as EremiteDesertClearwater;
  tags hydro, eremite;
  health 8;
  energy 2;
  skills AgileThrust,
    BlitzStrike,
    WhirlingLashes,
    OminousSpiritUnleashedHydro,
    WhirlingLashesFirstStrike,
    WhirlingLashesFixedPattern;
};
