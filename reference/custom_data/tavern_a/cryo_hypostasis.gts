// Cryo Hypostasis (4106 normal, 4107 hard): Tavern Challenge boss versions of Piovium's 2103.
// Same active skills; the Crystal Core heals to 6 / 10 HP and is regained once per match, and the boss is an
// Elemental Lifeform (permanent Cryo aura, immune to Cryo DMG and Frozen).
import { $, Aura, DamageType, Reaction } from "@gi-tcg/core/data";
import { Frozen } from "../../commons.gts";
import {
  IcespikeShot,
  IceRingWaltz,
  PlungingIceShards,
} from "../../characters/cryo/cryo_hypostasis.gts";

/**
 * @id 121035
 * @name Cryo Crystal Core
 * When the character would be defeated: remove this effect, prevent the defeat and heal them to 6 HP.
 */
define status {
  id 121035 as CryoCrystalCoreNormal;
  on beforeDefeated {
    :immune(6);
    :dispose();
  };
};

/**
 * @id 121036
 * @name Cryo Crystal Core
 * When the character would be defeated: remove this effect, prevent the defeat and heal them to 10 HP.
 */
define status {
  id 121036 as CryoCrystalCoreHard;
  on beforeDefeated {
    :immune(10);
    :dispose();
  };
};

/**
 * @id 121037
 * @name Cryo Crystal Core (Recovering)
 * When Action Phase begins: if the character has no Cryo Crystal Core (121035), it gains one. (Once per match)
 */
define status {
  id 121037 as CryoCrystalCoreRecoveringNormal;
  on actionPhase {
    when :( !:self.master.hasStatus(CryoCrystalCoreNormal) );
    :characterStatus(CryoCrystalCoreNormal, :self.master);
    :dispose();
  };
};

/**
 * @id 121038
 * @name Cryo Crystal Core (Recovering)
 * When Action Phase begins: if the character has no Cryo Crystal Core (121036), it gains one. (Once per match)
 */
define status {
  id 121038 as CryoCrystalCoreRecoveringHard;
  on actionPhase {
    when :( !:self.master.hasStatus(CryoCrystalCoreHard) );
    :characterStatus(CryoCrystalCoreHard, :self.master);
    :dispose();
  };
};

/**
 * @id 131071
 * @name Elemental Lifeform: Cryo
 * This character always has Cryo applied, and it is immune to Cryo DMG and Frozen.
 */
define status {
  id 131071 as ElementalLifeformCryo;
  on decreaseDamaged {
    when :( :e.type === DamageType.Cryo );
    :e.decreaseDamage(:e.value);
  };
  on reaction {
    if (:e.type === Reaction.Frozen) {
      :dispose($.typeStatus.def(Frozen).at(:self.master));
    }
    const aura = :self.master.aura;
    if (aura !== Aura.Cryo && aura !== Aura.CryoDendro) {
      :self.master.setVariable("aura", Aura.Cryo);
    }
  };
};

/**
 * @id 21037
 * @name Cryo Crystal Core
 * (Passive) When the battle begins, starts with Cryo Crystal Core (121035), regained once per match.
 */
define skill {
  id 21037 as CryoCrystalCoreSkillNormal;
  skillType passive {
    on battleBegin {
      :characterStatus(CryoCrystalCoreNormal);
      :characterStatus(CryoCrystalCoreRecoveringNormal);
    };
  };
};

/**
 * @id 21038
 * @name Cryo Crystal Core
 * (Passive) When the battle begins, starts with Cryo Crystal Core (121036), regained once per match.
 */
define skill {
  id 21038 as CryoCrystalCoreSkillHard;
  skillType passive {
    on battleBegin {
      :characterStatus(CryoCrystalCoreHard);
      :characterStatus(CryoCrystalCoreRecoveringHard);
    };
  };
};

/**
 * @id 31073
 * @name Elemental Sedimentation: Cryo
 * (Passive) When the battle begins, this character gains Elemental Lifeform: Cryo.
 */
define skill {
  id 31073 as ElementalSedimentationCryo;
  skillType passive {
    on battleBegin {
      :characterStatus(ElementalLifeformCryo);
      :self.setVariable("aura", Aura.Cryo);
    };
  };
};

define character {
  id 4106 as TavernCryoHypostasis;
  tags cryo, monster;
  health 30;
  energy 2;
  skills IcespikeShot,
    IceRingWaltz,
    PlungingIceShards,
    CryoCrystalCoreSkillNormal,
    ElementalSedimentationCryo;
};

define character {
  id 4107 as TavernCryoHypostasisHard;
  tags cryo, monster;
  health 45;
  energy 2;
  skills IcespikeShot,
    IceRingWaltz,
    PlungingIceShards,
    CryoCrystalCoreSkillHard,
    ElementalSedimentationCryo;
};
