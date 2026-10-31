// Skills shared by the Nobushi kits (Hitsukeban 3305, Kikouban 3405): the game gives both the same ids.
// Iai: Sheathed prepares Iai: Thrust over two turns, with Iai: Breathing (30035) as the intermediate
// prepared step, as the game's statuses 130031 and 130032 describe. 6 Physical DMG from the fandom wiki.
import { DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 30034
 * @name Iai: Thrust
 * (Prepare for 2 turns) Deals 6 Physical DMG.
 */
define skill {
  id 30034 as IaiThrust;
  skillType burst;
  prepared;
  :damage(DamageType.Physical, 6);
};

/**
 * @id 130032
 * @name Iai: Breathing
 * The next time this character acts, they will immediately use Iai: Thrust.
 */
define status {
  id 130032 as IaiBreathingStatus;
  prepare IaiThrust;
};

/**
 * @id 30035
 * @name Iai: Breathing
 * (Prepared for 1 Turn) The next time this character acts, they will use Iai: Thrust.
 */
define skill {
  id 30035 as IaiBreathing;
  skillType burst;
  prepared;
  :characterStatus(IaiBreathingStatus, :self);
};

/**
 * @id 130031
 * @name Iai: Sheathed
 * This character will immediately use Iai: Thrust the second time they act after the preparation starts.
 */
define status {
  id 130031 as IaiSheathedStatus;
  prepare IaiBreathing;
};

/**
 * @id 30033
 * @name Iai: Sheathed
 * Prepare Skill: Iai: Thrust.
 */
define skill {
  id 30033 as IaiSheathed;
  skillType burst;
  cost DiceType.Aligned, 3;
  cost DiceType.Energy, 2;
  :characterStatus(IaiSheathedStatus, :self);
};

/**
 * @id 30036
 * @name Zanshin
 * (Passive) End Phase: If this character is the active character, they gain 1 Energy.
 */
define skill {
  id 30036 as Zanshin;
  skillType passive {
    on endPhase {
      when :( :self.isActive() );
      :gainEnergy(1, :self);
    };
  };
};
