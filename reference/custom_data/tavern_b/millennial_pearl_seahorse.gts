// Millennial Pearl Seahorse, Tavern Challenge boss (4406 normal, 4407 hard).
// Its Fontemer Pearl (124033) never spends Usage on Summon DMG and gives +1 DMG while it lasts.
import { DamageType, DiceType } from "@gi-tcg/core/data";
import { TailSweep, ResonantCoralOrb } from "../../characters/electro/millennial_pearl_seahorse.gts";

/**
 * @id 124033
 * @name Fontemer Pearl
 * @description
 * When character to which this is attached takes DMG: Negate 1 DMG. Usage(s) will not be used when negating DMG from Summons.
 * Usage(s): 2
 * During this effect's duration: The character to which this is attached deals +1 DMG.
 */
define status {
  id 124033 as FontemerPearlTavern;
  tags barrier;
  on decreaseDamaged {
    usage 2 {
      autoDecrease false;
    };
    :e.decreaseDamage(1);
    if (:e.source.definition.type !== "summon") {
      :consumeUsage();
    }
  };
  on increaseSkillDamage {
    :e.increaseDamage(1);
  };
  // Swirling School of Fish: "If this character has Fontemer Pearl attached, that Usage(s) +1. (Once per Round)"
  on useSkill {
    when :( :e.skill.definition.id === SwirlingSchoolOfFishTavern );
    usage perRound, 1 {
      name "usagePerRound1";
    };
    :addVariable("usage", 1);
  };
};

/**
 * @id 24038
 * @name Swirling School of Fish
 * @cost 3*Electro
 * @description
 * Deals 3 Electro DMG.
 * If this character has Fontemer Pearl attached, that Usage(s) +1. (Once per Round)
 */
define skill {
  id 24038 as SwirlingSchoolOfFishTavern;
  skillType elemental;
  cost DiceType.Electro, 3;
  :damage(DamageType.Electro, 3);
};

/**
 * @id 24036
 * @name Fontemer Hoarthunder
 * @cost 3*Electro, 2*Energy
 * @description
 * Deals 1 Electro DMG, attaches Fontemer Pearl to this character, and summons 1 Resonant Coral Orb.
 */
define skill {
  id 24036 as FontemerHoarthunderTavern;
  skillType burst;
  cost DiceType.Electro, 3;
  cost DiceType.Energy, 2;
  :damage(DamageType.Electro, 1);
  :characterStatus(FontemerPearlTavern);
  :summon(ResonantCoralOrb);
};

/**
 * @id 24035
 * @name Pearl Armor
 * @description
 * (Passive) When the battle begins, attach Fontemer Pearl to this character.
 */
define skill {
  id 24035 as PearlArmorTavern;
  skillType passive {
    on battleBegin {
      :characterStatus(FontemerPearlTavern);
    };
  };
};

define character {
  id 4406 as MillennialPearlSeahorseNormal;
  tags electro, monster;
  health 18;
  energy 2;
  skills TailSweep, SwirlingSchoolOfFishTavern, FontemerHoarthunderTavern, PearlArmorTavern;
};

define character {
  id 4407 as MillennialPearlSeahorseHard;
  tags electro, monster;
  health 27;
  energy 2;
  skills TailSweep, SwirlingSchoolOfFishTavern, FontemerHoarthunderTavern, PearlArmorTavern;
};
