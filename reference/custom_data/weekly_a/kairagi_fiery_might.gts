// Kairagi: Fiery Might (3303). Damage values from the fandom wiki skill pages.
// Iai: Sheathed prepares for 2 turns: the first prepared turn uses Iai: Breathing (no effect), the second
// uses Iai: Fiery Wind, mirroring Piovium's chained prepare statuses (nextStatus).
// Piovium has no Kairagi tag, so Final Frenzy recognises Kairagi characters by id (3303, 3401).
import { DamageType, DiceType } from "@gi-tcg/core/data";

const KAIRAGI_CHARACTER_IDS: number[] = [3303, 3401];

/**
 * @id 133031
 * @name Kamuna Talisman: Pyro
 * @description
 * The Physical DMG this character deals is converted to Pyro DMG.
 * Usage(s): 3
 */
define status {
  id 133031 as KamunaTalismanPyro;
  on modifySkillDamageType {
    when :( :e.type === DamageType.Physical );
    usage 3;
    :e.changeDamageType(DamageType.Pyro);
  };
};

/**
 * @id 133032
 * @name Final Frenzy
 * @description
 * When the Action Phase starts: If the character to which this is attached is the active character,
 * remove this status, heal this character for 3 HP, and re-attach Kamuna Talisman: Pyro.
 */
define status {
  id 133032 as FinalFrenzy;
  on actionPhase {
    when :( :self.master.isActive() );
    const master = :self.master;
    :dispose();
    :heal(3, master);
    :characterStatus(KamunaTalismanPyro, master);
  };
};

/**
 * @id 33036
 * @name Iai: Fiery Wind
 * @description
 * (Prepare for 2 turns)
 * Deals 6 Physical DMG.
 */
define skill {
  id 33036 as IaiFieryWind;
  skillType burst;
  prepared;
  :damage(DamageType.Physical, 6);
};

/**
 * @id 33037
 * @name Iai: Breathing
 * @description
 * (Prepared for 1 Turn)
 * The next time this character acts, they will use the Skill Iai: Fiery Wind.
 */
define skill {
  id 33037 as KairagiIaiBreathing;
  skillType burst;
  prepared;
};

/**
 * @id 133034
 * @name Iai: Sheathed
 * @description
 * The next time this character acts, they will immediately use the Skill Iai: Fiery Wind.
 */
define status {
  id 133034 as IaiFieryWindStatus;
  prepare IaiFieryWind;
};

/**
 * @id 133033
 * @name Iai: Sheathed
 * @description
 * This character will immediately use the Skill Iai: Fiery Wind the second time they act after the preparation starts.
 */
define status {
  id 133033 as KairagiIaiSheathedStatus;
  prepare KairagiIaiBreathing {
    nextStatus IaiFieryWindStatus;
  };
};

/**
 * @id 33031
 * @name Three Point Slash
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 33031 as ThreePointSlash;
  skillType normal;
  cost DiceType.Pyro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 33032
 * @name Leaping Strike
 * @description
 * Deals 3 Physical DMG.
 */
define skill {
  id 33032 as LeapingStrike;
  skillType elemental;
  cost DiceType.Pyro, 3;
  :damage(DamageType.Physical, 3);
};

/**
 * @id 33033
 * @name Iai: Sheathed
 * @description
 * Prepare Skill: Iai: Fiery Wind.
 */
define skill {
  id 33033 as KairagiIaiSheathed;
  skillType burst;
  cost DiceType.Pyro, 3;
  cost DiceType.Energy, 2;
  :characterStatus(KairagiIaiSheathedStatus);
};

/**
 * @id 33034
 * @name Kamuna Talisman: Pyro
 * @description
 * (Passive) When the battle begins, this character gains Kamuna Talisman: Pyro.
 */
define skill {
  id 33034 as KamunaTalismanPyroPassive;
  skillType passive {
    on battleBegin {
      :characterStatus(KamunaTalismanPyro, :self);
    };
  };
};

/**
 * @id 33035
 * @name Final Frenzy
 * @description
 * When another of your Kairagi characters gets defeated, this character gains Final Frenzy.
 */
define skill {
  id 33035 as FinalFrenzyPassive;
  skillType passive {
    on defeated {
      when :(
        :e.target.isMine() &&
          :e.target.id !== :self.id &&
          KAIRAGI_CHARACTER_IDS.includes(:e.target.definition.id)
      );
      listenTo all;
      :characterStatus(FinalFrenzy, :self);
    };
  };
};

define character {
  id 3303 as KairagiFieryMight;
  tags pyro;
  health 10;
  energy 2;
  skills ThreePointSlash, LeapingStrike, KairagiIaiSheathed, KamunaTalismanPyroPassive, FinalFrenzyPassive,
    IaiFieryWind, KairagiIaiBreathing;
};
