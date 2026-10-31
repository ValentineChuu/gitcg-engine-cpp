// Pyro Whopperflower (3309): NPC-only monster written from the game description.
// Damage values: fandom wiki skill pages (Fierce Headbutt: 2 Physical, Scorching Leaves: 3 Pyro,
// Roasted Nectar Burst: 7 Pyro).
// Masterful Mimicry: at the start of each Round the opponent picks 1 of 3 cards: 2 random real Action
// Cards (non-talent, discoverable) and 1 random Whopperflower's Disguise (game cards 133085-133099,
// see whopperflowers_disguise.gts), in random order. A real card goes to the opponent's hand. A disguise
// is removed from hand and the Whopperflower gains 1 Energy. The wiki does not say where the 2 real
// cards come from; drawing them from all discoverable Action Cards is an interpretation.
// The selection must be made by the opponent, so the passive places a combat status on the opponent's
// side that makes the offer. The game gives it no id; 133084 is an unused id near this character's range
// (133085-133099 are the disguises themselves).
import { DamageType, DiceType } from "@gi-tcg/core/data";
import { isWhopperflowersDisguise, WhopperflowersDisguises } from "./whopperflowers_disguise.gts";

// With several Whopperflowers on a team, only the first alive one handles a disguise (disposing the card
// twice would fail). A plain helper: `:self` inside nested arrow functions in `when` compiles wrongly.
function handlesDisguise(selfId: number, characters: readonly { id: number; definition: { id: number }; variables: { alive: number } }[]): boolean {
  const first = characters.find((character) => character.definition.id === 3309 && character.variables.alive);
  return first?.id === selfId;
}

function isRealActionCard(card: { type: string; tags: readonly string[] }): boolean {
  return ["eventCard", "support", "equipment"].includes(card.type) && !card.tags.includes("talent");
}

/**
 * @id 133084
 * @name Masterful Mimicry: Offer
 * @description
 * Pick 1 of 3 cards. If it is the Whopperflower's Disguise, the opposing Whopperflower gains 1 Energy.
 */
define combatStatus {
  id 133084 as MasterfulMimicryOffer;
  on selfEnter {
    const realCards = :randomSubset(:allCardDefinitions(isRealActionCard), 2);
    const disguise = :random(WhopperflowersDisguises);
    const offer = [...realCards];
    offer.splice(:random([0, 1, 2]), 0, disguise);
    :selectAndCreateHandCard(offer);
  };
  on selectCard {
    :dispose();
  };
};

/**
 * @id 33091
 * @name Fierce Headbutt
 * @description
 * Deals 2 Physical DMG.
 */
define skill {
  id 33091 as FierceHeadbutt;
  skillType normal;
  cost DiceType.Pyro, 1;
  cost DiceType.Void, 2;
  :damage(DamageType.Physical, 2);
};

/**
 * @id 33092
 * @name Scorching Leaves
 * @description
 * Deals 3 Pyro DMG.
 */
define skill {
  id 33092 as ScorchingLeaves;
  skillType elemental;
  cost DiceType.Pyro, 3;
  :damage(DamageType.Pyro, 3);
};

/**
 * @id 33093
 * @name Roasted Nectar Burst
 * @description
 * Deals 7 Pyro DMG.
 */
define skill {
  id 33093 as RoastedNectarBurst;
  skillType burst;
  cost DiceType.Pyro, 3;
  cost DiceType.Energy, 3;
  :damage(DamageType.Pyro, 7);
};

/**
 * @id 33094
 * @name Masterful Mimicry
 * @description
 * At the start of the Round: The opponent must Select 1 of 3 random cards. If they choose the
 * Whopperflower's Disguise, the Whopperflower gains 1 Energy.
 */
define skill {
  id 33094 as MasterfulMimicry;
  skillType passive {
    on roundBegin {
      :combatStatus(MasterfulMimicryOffer, "opp");
    };
    on handCardInserted {
      listenTo all;
      when :( :e.who !== :self.who && isWhopperflowersDisguise(:e.card.definition.id) && handlesDisguise(:self.id, :player.characters) );
      :dispose(:e.card);
      :gainEnergy(1, :self);
    };
  };
};

/**
 * @id 3309
 * @name Pyro Whopperflower
 * @hp 10
 * @energy 3
 */
define character {
  id 3309 as PyroWhopperflower;
  tags pyro, monster;
  health 10;
  energy 3;
  skills FierceHeadbutt, ScorchingLeaves, RoastedNectarBurst, MasterfulMimicry;
};
