// Lord of Eroded Primal Fire, Tavern Challenge boss (4308 normal, 4309 hard).
// Same kit as Piovium's 2305, plus Roar From the Darkness.
import {
  VoidClawStrike,
  ErodedFlamingFeathers,
  SeveringPrimalFire,
  ResentmentPassive,
} from "../../characters/pyro/lord_of_eroded_primal_fire.gts";

/**
 * @id 23055
 * @name Roar From the Darkness
 * @description
 * (Passive) When this character takes DMG: If this character's HP is not greater than 6, both players Discard the top 9 cards from their decks. (Once per match)
 */
define skill {
  id 23055 as RoarFromTheDarkness;
  skillType passive {
    on damaged {
      when :( :self.health <= 6 );
      usage 1 {
        name "roarUsage";
      };
      :abortPreview();
      for (const player of [:player, :oppPlayer]) {
        for (const card of player.pile.slice(0, 9).toReversed()) {
          :discard(card);
        }
      }
    };
  };
};

define character {
  id 4308 as LordOfErodedPrimalFireNormal;
  tags pyro, monster;
  health 18;
  energy 2;
  skills VoidClawStrike,
    ErodedFlamingFeathers,
    SeveringPrimalFire,
    ResentmentPassive,
    RoarFromTheDarkness;
};

define character {
  id 4309 as LordOfErodedPrimalFireHard;
  tags pyro, monster;
  health 28;
  energy 2;
  skills VoidClawStrike,
    ErodedFlamingFeathers,
    SeveringPrimalFire,
    ResentmentPassive,
    RoarFromTheDarkness;
};
