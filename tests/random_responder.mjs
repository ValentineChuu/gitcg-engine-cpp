// Seeded random answers to worker decisions, covering every request type (rerolls and mulligans included),
// so the reference games exercise as much of the engine as possible.

function pick(items, random) {
  return items[Math.floor(random() * items.length)];
}

function randomSubset(items, random) {
  return items.filter(() => random() < 0.5);
}

export function randomResponse(decision, random) {
  const { request, who } = decision;
  const me = decision.state.player[who];
  switch (request.$case) {
    case "action": {
      const valid = request.value.action.map((action, index) => [action, index]).filter(([action]) => (action.validity ?? 0) === 0);
      const [action, index] = pick(valid, random);
      return { $case: "action", value: { chosenActionIndex: index, usedDice: action.autoSelectedDice } };
    }
    case "chooseActive":
      return { $case: "chooseActive", value: { activeCharacterId: pick(request.value.candidateIds, random) } };
    case "selectCard":
      return { $case: "selectCard", value: { selectedDefinitionId: pick(request.value.candidateDefinitionIds, random) } };
    case "rerollDice":
      return { $case: "rerollDice", value: { diceToReroll: randomSubset(me.dice, random) } };
    case "switchHands":
      return { $case: "switchHands", value: { removedHandIds: randomSubset(me.handCard.map((card) => card.id), random) } };
  }
  throw new Error(`unknown request ${request.$case}`);
}
