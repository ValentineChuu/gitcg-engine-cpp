// Emperor of Fire and Iron, Tavern Challenge boss (4306 normal, 4307 hard).
// Same kit as Piovium's 2304, except its burst also grants 3 (normal) or 6 (hard) Armored Crab Carapace.
import { DiceType } from "@gi-tcg/core/data";
import {
  ArmoredCrabCarapace,
  AccruingPower,
  ShatterclampStrike,
  BusterBlaze,
  ImperialPanoply,
  SearingBlast,
} from "../../characters/pyro/emperor_of_fire_and_iron.gts";

/**
 * @id 23048
 * @name Battle-Line Detonation
 * @cost 3*Pyro, 2*Energy
 * @description
 * This character gains 3 stacks of Armored Crab Carapace and prepares: Searing Blast.
 */
define skill {
  id 23048 as BattlelineDetonationNormal;
  skillType burst;
  cost DiceType.Pyro, 3;
  cost DiceType.Energy, 2;
  :characterStatus(ArmoredCrabCarapace, :self, {
    overrideVariables: {
      shield: 3,
    },
  });
  :characterStatus(AccruingPower, :self);
};

/**
 * @id 23049
 * @name Battle-Line Detonation
 * @cost 3*Pyro, 2*Energy
 * @description
 * This character gains 6 stacks of Armored Crab Carapace and prepares: Searing Blast.
 */
define skill {
  id 23049 as BattlelineDetonationHard;
  skillType burst;
  cost DiceType.Pyro, 3;
  cost DiceType.Energy, 2;
  :characterStatus(ArmoredCrabCarapace, :self, {
    overrideVariables: {
      shield: 6,
    },
  });
  :characterStatus(AccruingPower, :self);
};

define character {
  id 4306 as EmperorOfFireAndIronNormal;
  tags pyro, monster;
  health 15;
  energy 2;
  skills ShatterclampStrike,
    BusterBlaze,
    BattlelineDetonationNormal,
    ImperialPanoply,
    SearingBlast;
};

define character {
  id 4307 as EmperorOfFireAndIronHard;
  tags pyro, monster;
  health 24;
  energy 2;
  skills ShatterclampStrike,
    BusterBlaze,
    BattlelineDetonationHard,
    ImperialPanoply,
    SearingBlast;
};
