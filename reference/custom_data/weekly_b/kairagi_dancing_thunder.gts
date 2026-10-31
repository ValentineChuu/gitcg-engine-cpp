// Kairagi: Dancing Thunder (3401): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages "Three Point Slash/Leaping Strike/Iai: Sheathed (Kairagi:
// Dancing Thunder)": 2 / 3 Physical and Iai: Stormbreath 6 Physical (Physical becomes Electro while
// Kamuna Talisman: Electro has usages).
// Two-turn preparation: Iai: Sheathed (134013) uses the hidden step Iai: Breathing (34017) on the next
// action, which attaches the second Iai: Sheathed (134014) that uses Iai: Stormbreath (34016).
import { DamageType, DiceType } from "@gi-tcg/core/data";

// Character ids of the game's Kairagi characters (camp_kairagi): Fiery Might and Dancing Thunder.
const KAIRAGI_CHARACTER_IDS = [3303, 3401];

/**
 * @id 134011
 * @name Kamuna Talisman: Electro
 * @description
 * The Physical DMG this character deals is converted to Electro DMG.
 * Usage(s): 3
 */
define status {
  id 134011 as KamunaTalismanElectro;
  on modifySkillDamageType {
    when :( :e.type === DamageType.Physical );
    usage 3;
    :e.changeDamageType(DamageType.Electro);
  };
};

/**
 * @id 134012
 * @name Final Frenzy
 * @description
 * When the Action Phase starts: If the character to which this is attached is the active character,
 * remove this status, heal this character for 3 HP, and re-attach Kamuna Talisman: Electro.
 */
define status {
  id 134012 as FinalFrenzy;
  on actionPhase {
    when :( :self.master.isActive() );
    :heal(3, :self.master);
    :characterStatus(KamunaTalismanElectro, :self.master);
    :dispose();
  };
};

/**
 * @id 34016
 * @name Iai: Stormbreath
 * @description
 * (Prepare for 2 turns)
 * Deals 6 Physical DMG.
 */
define skill {
  id 34016 as IaiStormbreath;
  skillType burst;
  prepared;
  :damage(DamageType.Physical, 6);
};

/**
 * @id 134014
 * @name Iai: Sheathed
 * @description
 * The next time this character acts, they will immediately use the Skill Iai: Stormbreath.
 */
define status {
  id 134014 as IaiSheathedFinalTurn;
  prepare IaiStormbreath;
};

/**
 * @id 34017
 * @name Iai: Breathing
 * @description
 * (Prepared for 1 Turn)
 * The next time this character acts, they will use the Skill Iai: Stormbreath.
 */
define skill {
  id 34017 as IaiBreathing;
  skillType burst;
  prepared;
  :characterStatus(IaiSheathedFinalTurn, :self);
};

/**
 * @id 134013
 * @name Iai: Sheathed
 * @description
 * This character will immediately use the Skill Iai: Stormbreath the second time they act after the
 * preparation starts.
 */
define status {
  id 134013 as IaiSheathedFirstTurn;
  prepare IaiBreathing;
};

/**
 * @id 34011
 * @name Three Point Slash
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 34011 as ThreePointSlash;
  skillType normal;
  cost DiceType.Electro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 34012
 * @name Leaping Strike
 * @description
 * Deals 3 Physical DMG.
 */
define skill {
  id 34012 as LeapingStrike;
  skillType elemental;
  cost DiceType.Electro, 3;
  :damage(DamageType.Physical, 3);
};

/**
 * @id 34013
 * @name Iai: Sheathed
 * @description
 * Prepares Iai: Stormbreath.
 */
define skill {
  id 34013 as IaiSheathed;
  skillType burst;
  cost DiceType.Electro, 3;
  cost DiceType.Energy, 2;
  :characterStatus(IaiSheathedFirstTurn, :self);
};

/**
 * @id 34014
 * @name Kamuna Talisman: Electro
 * @description
 * (Passive) When the battle begins, this character gains Kamuna Talisman: Electro.
 */
define skill {
  id 34014 as KamunaTalismanElectroPassive;
  skillType passive {
    on battleBegin {
      :characterStatus(KamunaTalismanElectro);
    };
  };
};

/**
 * @id 34015
 * @name Final Frenzy
 * @description
 * When another of your Kairagi characters gets defeated, this character gains Final Frenzy.
 */
define skill {
  id 34015 as FinalFrenzyPassive;
  skillType passive {
    on defeated {
      listenTo samePlayer;
      when :(
        :e.target.id !== :self.id &&
          :self.variables.alive === 1 &&
          KAIRAGI_CHARACTER_IDS.includes(:e.target.definition.id)
      );
      :characterStatus(FinalFrenzy, :self);
    };
  };
};

/**
 * @id 3401
 * @name Kairagi: Dancing Thunder
 * @hp 10
 * @energy 2
 */
define character {
  id 3401 as KairagiDancingThunder;
  tags electro;
  health 10;
  energy 2;
  skills ThreePointSlash,
    LeapingStrike,
    IaiSheathed,
    KamunaTalismanElectroPassive,
    FinalFrenzyPassive,
    IaiStormbreath,
    IaiBreathing;
};
