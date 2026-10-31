// Whopperflower's Disguise cards for the Pyro Whopperflower's Masterful Mimicry.
// Ids, names and costs come from GCGCardExcelConfigData (133085-133099). The game marks them
// non-discoverable; their names copy real cards with small changes ("Sweet Ma'am", "Amos' Bow",
// "Tomb of the Eternal Flow", ...), which is how a careful player sees through the disguise.
// They are defined as effect-less event cards: one is offered next to two real cards, and
// Masterful Mimicry removes it from hand as soon as it is chosen.
import { DiceType } from "@gi-tcg/core/data";

/**
 * @id 133085
 * @name Tandoori Grilled Chicken
 */
define card {
  id 133085 as DisguiseTandooriGrilledChicken;
  undiscoverable;
  cost DiceType.Void, 2;
};

/**
 * @id 133086
 * @name Tenacity of the Millelith
 */
define card {
  id 133086 as DisguiseTenacityOfTheMillelith;
  undiscoverable;
  cost DiceType.Aligned, 3;
};

/**
 * @id 133087
 * @name Former Site of the Central Laboratory
 */
define card {
  id 133087 as DisguiseFormerSiteOfTheCentralLaboratory;
  undiscoverable;
  cost DiceType.Aligned, 1;
};

/**
 * @id 133088
 * @name Favonius Cathedral
 */
define card {
  id 133088 as DisguiseFavoniusCathedral;
  undiscoverable;
  cost DiceType.Aligned, 2;
};

/**
 * @id 133089
 * @name Skyward Sword
 */
define card {
  id 133089 as DisguiseSkywardSword;
  undiscoverable;
  cost DiceType.Aligned, 3;
};

/**
 * @id 133090
 * @name Underwater Treasure Hunt
 */
define card {
  id 133090 as DisguiseUnderwaterTreasureHunt;
  undiscoverable;
  cost DiceType.Aligned, 2;
};

/**
 * @id 133091
 * @name Controlled Directional Blast
 */
define card {
  id 133091 as DisguiseControlledDirectionalBlast;
  undiscoverable;
  cost DiceType.Aligned, 1;
};

/**
 * @id 133092
 * @name King's Valet
 */
define card {
  id 133092 as DisguiseKingsValet;
  undiscoverable;
  cost DiceType.Aligned, 3;
};

/**
 * @id 133093
 * @name Amos' Bow
 */
define card {
  id 133093 as DisguiseAmossBow;
  undiscoverable;
  cost DiceType.Aligned, 3;
};

/**
 * @id 133094
 * @name The Tale of Vennessa
 */
define card {
  id 133094 as DisguiseTheTaleOfVennessa;
  undiscoverable;
  cost DiceType.Aligned, 3;
};

/**
 * @id 133095
 * @name Gilded Dreams
 */
define card {
  id 133095 as DisguiseGildedDreams;
  undiscoverable;
  cost DiceType.Aligned, 3;
};

/**
 * @id 133096
 * @name Lumenarystone
 */
define card {
  id 133096 as DisguiseLumenarystone;
  undiscoverable;
  cost DiceType.Void, 3;
};

/**
 * @id 133097
 * @name Sweet Ma'am
 */
define card {
  id 133097 as DisguiseSweetMaam;
  undiscoverable;
};

/**
 * @id 133098
 * @name Delicious Macarons
 */
define card {
  id 133098 as DisguiseDeliciousMacarons;
  undiscoverable;
  cost DiceType.Void, 2;
};

/**
 * @id 133099
 * @name Tomb of the Eternal Flow
 */
define card {
  id 133099 as DisguiseTombOfTheEternalFlow;
  undiscoverable;
  cost DiceType.Aligned, 3;
};

export const WhopperflowersDisguises = [
  DisguiseTandooriGrilledChicken,
  DisguiseTenacityOfTheMillelith,
  DisguiseFormerSiteOfTheCentralLaboratory,
  DisguiseFavoniusCathedral,
  DisguiseSkywardSword,
  DisguiseUnderwaterTreasureHunt,
  DisguiseControlledDirectionalBlast,
  DisguiseKingsValet,
  DisguiseAmossBow,
  DisguiseTheTaleOfVennessa,
  DisguiseGildedDreams,
  DisguiseLumenarystone,
  DisguiseSweetMaam,
  DisguiseDeliciousMacarons,
  DisguiseTombOfTheEternalFlow,
];

export function isWhopperflowersDisguise(cardDefinitionId: number): boolean {
  return cardDefinitionId >= 133085 && cardDefinitionId <= 133099;
}
