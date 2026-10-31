// Ruin Drake: Earthguard (3005). Damage values from the fandom wiki skill pages.
// Elemental Absorption: after taking Elemental DMG while nothing is absorbed, the character gains the
// matching Absorption Effect status (130051-130057), which halves DMG of that element (rounded up).
// Energy Surge consumes the absorbed element: +1 DMG and the DMG type becomes that element.
// The hidden passive 30056 is omitted; 30055 carries the whole absorption mechanic.
import { DamageType, DiceType, type StatusHandle } from "@gi-tcg/core/data";

/**
 * @id 130051
 * @name Absorption Effect: Cryo
 * @description
 * Character has absorbed Cryo. Cryo DMG taken is halved. (Rounded up)
 */
define status {
  id 130051 as AbsorptionEffectCryo;
  on multiplyDamaged {
    when :( :e.type === DamageType.Cryo );
    :e.divideDamage(2);
  };
};

/**
 * @id 130052
 * @name Absorption Effect: Hydro
 * @description
 * Character has absorbed Hydro. Hydro DMG taken is halved. (Rounded up)
 */
define status {
  id 130052 as AbsorptionEffectHydro;
  on multiplyDamaged {
    when :( :e.type === DamageType.Hydro );
    :e.divideDamage(2);
  };
};

/**
 * @id 130053
 * @name Absorption Effect: Pyro
 * @description
 * Character has absorbed Pyro. Pyro DMG taken is halved. (Rounded up)
 */
define status {
  id 130053 as AbsorptionEffectPyro;
  on multiplyDamaged {
    when :( :e.type === DamageType.Pyro );
    :e.divideDamage(2);
  };
};

/**
 * @id 130054
 * @name Absorption Effect: Electro
 * @description
 * Character has absorbed Electro. Electro DMG taken is halved. (Rounded up)
 */
define status {
  id 130054 as AbsorptionEffectElectro;
  on multiplyDamaged {
    when :( :e.type === DamageType.Electro );
    :e.divideDamage(2);
  };
};

/**
 * @id 130055
 * @name Absorption Effect: Anemo
 * @description
 * Character has absorbed Anemo. Anemo DMG taken is halved. (Rounded up)
 */
define status {
  id 130055 as AbsorptionEffectAnemo;
  on multiplyDamaged {
    when :( :e.type === DamageType.Anemo );
    :e.divideDamage(2);
  };
};

/**
 * @id 130056
 * @name Absorption Effect: Geo
 * @description
 * Character has absorbed Geo. Geo DMG taken is halved. (Rounded up)
 */
define status {
  id 130056 as AbsorptionEffectGeo;
  on multiplyDamaged {
    when :( :e.type === DamageType.Geo );
    :e.divideDamage(2);
  };
};

/**
 * @id 130057
 * @name Absorption Effect: Dendro
 * @description
 * Character has absorbed Dendro. Dendro DMG taken is halved. (Rounded up)
 */
define status {
  id 130057 as AbsorptionEffectDendro;
  on multiplyDamaged {
    when :( :e.type === DamageType.Dendro );
    :e.divideDamage(2);
  };
};

const ABSORPTION_EFFECTS: [StatusHandle, DamageType][] = [
  [AbsorptionEffectCryo, DamageType.Cryo],
  [AbsorptionEffectHydro, DamageType.Hydro],
  [AbsorptionEffectPyro, DamageType.Pyro],
  [AbsorptionEffectElectro, DamageType.Electro],
  [AbsorptionEffectAnemo, DamageType.Anemo],
  [AbsorptionEffectGeo, DamageType.Geo],
  [AbsorptionEffectDendro, DamageType.Dendro],
];

// Plain helpers: GTS mis-compiles `:self` inside nested arrow functions within `when`.
function findAbsorbed(character: { hasStatus(status: StatusHandle): unknown }) {
  return ABSORPTION_EFFECTS.find(([status]) => character.hasStatus(status));
}

function absorptionStatusFor(element: DamageType) {
  return ABSORPTION_EFFECTS.find(([, absorbedElement]) => absorbedElement === element)?.[0];
}

/**
 * @id 30051
 * @name Close Expulsion
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 30051 as CloseExpulsion;
  skillType normal;
  cost DiceType.Void, 3;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 30052
 * @name Sudden Charge
 * @description
 * Deals 3 Physical DMG.
 */
define skill {
  id 30052 as SuddenCharge;
  skillType elemental;
  cost DiceType.Aligned, 3;
  :damage(DamageType.Physical, 3);
};

/**
 * @id 30053
 * @name Energy Surge
 * @description
 * Deals 5 Physical DMG. If this character has absorbed Elemental Energy, it will release that element
 * for +1 DMG and change DMG type to the absorbed element.
 */
define skill {
  id 30053 as EnergySurge;
  skillType burst;
  cost DiceType.Aligned, 3;
  cost DiceType.Energy, 2;
  const absorbed = findAbsorbed(:self);
  if (absorbed) {
    :damage(absorbed[1], 6);
    :self.hasStatus(absorbed[0])?.dispose();
  } else {
    :damage(DamageType.Physical, 5);
  }
};

/**
 * @id 30054
 * @name Standard Armor
 * @description
 * (Passive) The Physical DMG this character receives is halved (Rounded up)
 */
define skill {
  id 30054 as RuinDrakeStandardArmor;
  skillType passive {
    on multiplyDamaged {
      when :( :e.type === DamageType.Physical );
      :e.divideDamage(2);
    };
  };
};

/**
 * @id 30055
 * @name Elemental Absorption
 * @description
 * (Passive) When no element has been absorbed and this character takes Elemental DMG, absorb that element.
 * Halve DMG of that element taken by this character. (Rounded up)
 */
define skill {
  id 30055 as ElementalAbsorption;
  skillType passive {
    on damaged {
      when :( !!absorptionStatusFor(:e.type) && !findAbsorbed(:self) );
      :characterStatus(absorptionStatusFor(:e.type)!, :self);
    };
  };
};

define character {
  id 3005 as RuinDrakeEarthguard;
  tags monster;
  health 10;
  energy 2;
  skills CloseExpulsion, SuddenCharge, EnergySurge, RuinDrakeStandardArmor, ElementalAbsorption;
};
