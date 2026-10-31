// Goldflame Qucusaur Tyrant, Tavern Challenge boss (4310 normal, 4311 hard).
// Same attacks as Piovium's 2306; its Ancient Bloodline triggers every Action Phase and also creates a 2-cost card.
import type { CardHandle } from "@gi-tcg/core/data";
import {
  GoldflameState,
  Wingcleave,
  HotRisingWind,
  GoldflameExplosion,
} from "../../characters/pyro/goldflame_qucusaur_tyrant.gts";

/**
 * @id 23065
 * @name Ancient Bloodline
 * @description
 * (Passive) When Action Phase begins, attach 1 stack of Goldflame State to self while also generating 1 random card that costs 2 Elemental Dice.
 */
define skill {
  id 23065 as AncientBloodlineTavern;
  skillType passive {
    on actionPhase {
      :characterStatus(GoldflameState, :self);
      // Pool: obtainable action cards (not talents) whose play cost is exactly 2 dice.
      const candidates = :allCardDefinitions(
        (card) =>
          !card.tags.includes("talent") &&
          card.skills.some(
            (skill) =>
              skill.skillType === "playCard" &&
              skill.initiativeSkillConfig?.computed$diceCostSize === 2,
          ),
      );
      const chosen = :random(candidates);
      if (chosen) {
        :createHandCard(chosen.id as CardHandle);
      }
    };
  };
};

define character {
  id 4310 as GoldflameQucusaurTyrantNormal;
  tags pyro, monster;
  health 16;
  energy 2;
  skills Wingcleave, HotRisingWind, GoldflameExplosion, AncientBloodlineTavern;
};

define character {
  id 4311 as GoldflameQucusaurTyrantHard;
  tags pyro, monster;
  health 24;
  energy 2;
  skills Wingcleave, HotRisingWind, GoldflameExplosion, AncientBloodlineTavern;
};
