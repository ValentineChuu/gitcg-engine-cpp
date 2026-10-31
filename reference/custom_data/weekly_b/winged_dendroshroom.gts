// Winged Dendroshroom (3701): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages (Spore Bullet: 1 Dendro, Rolling Spray: 3 Dendro,
// Divebomb: 5 Dendro).
// The game's onstage 137014 "Activated" (no description) is not defined: the energy on defeat is handled
// by the character status 137013 itself.
import { $, DamageType, DiceType } from "@gi-tcg/core/data";

/**
 * @id 137012
 * @name Scorched
 * @description
 * After this character becomes Scorched, it will lose its original buff.
 */
define status {
  id 137012 as Scorched;
};

/**
 * @id 137013
 * @name Activated
 * @description
 * End Phase: The character gains 1 Energy.
 * After the character goes down: The opposing active character gains 1 Energy.
 */
define status {
  id 137013 as Activated;
  on endPhase {
    :gainEnergy(1, :self.master);
  };
  on defeated {
    when :( :e.target.id === :self.master.id );
    :gainEnergy(1, $.opp.active);
  };
};

/**
 * @id 137011
 * @name Aerial Superiority
 * @description
 * The character takes -1 DMG. (This character will permanently change its State if it takes Pyro DMG or
 * Electro DMG.)
 */
define status {
  id 137011 as AerialSuperiority;
  on decreaseDamaged {
    when :( :e.value > 0 );
    :e.decreaseDamage(1);
  };
  on damaged {
    when :( :e.type === DamageType.Pyro || :e.type === DamageType.Electro );
    if (:e.type === DamageType.Pyro) {
      :characterStatus(Scorched, :self.master);
    } else {
      :characterStatus(Activated, :self.master);
    }
    :dispose();
  };
};

/**
 * @id 37011
 * @name Spore Bullet
 * @description
 * Deals 1 Dendro DMG.
 */
define skill {
  id 37011 as SporeBullet;
  skillType normal;
  cost DiceType.Dendro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Dendro, 1);
};

/**
 * @id 37012
 * @name Rolling Spray
 * @description
 * Deals 3 Dendro DMG.
 */
define skill {
  id 37012 as RollingSpray;
  skillType elemental;
  cost DiceType.Dendro, 3;
  :damage(DamageType.Dendro, 3);
};

/**
 * @id 37013
 * @name Divebomb
 * @description
 * Deals 5 Dendro DMG.
 */
define skill {
  id 37013 as Divebomb;
  skillType burst;
  cost DiceType.Dendro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Dendro, 5);
};

/**
 * @id 37014
 * @name State Change
 * @description
 * (Passive) When the battle begins, the character will be in a Aerial Superiority.
 * After this character takes Pyro DMG, it will become Scorched.
 * After this character takes Electro DMG, it will become Activated.
 */
define skill {
  id 37014 as StateChange;
  skillType passive {
    on battleBegin {
      :characterStatus(AerialSuperiority);
    };
  };
};

/**
 * @id 3701
 * @name Winged Dendroshroom
 * @hp 6
 * @energy 2
 */
define character {
  id 3701 as WingedDendroshroom;
  tags dendro, monster;
  health 6;
  energy 2;
  skills SporeBullet, RollingSpray, Divebomb, StateChange;
};
