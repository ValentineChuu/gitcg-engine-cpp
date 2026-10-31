// What the end of this round would do if both players ended it now (network input "end of round"): one
// copy per action decision, hidden cards re-dealt as for replies (redeal.mjs) so nothing hidden leaks; in the copy
// the deciding player ends the round, the opponent ends it at their next chance, and the end phase runs (summons and
// statuses act, healing, defeats). Read when the next round's first decision comes, or when the game ends there.
// Returns per player (in seat order) the HP change and defeat of each character, and the winner if the game ended.

import { Game } from "@gi-tcg/core";
import { engineWork } from "./engine_work.mjs";
import { redealHiddenCards } from "./redeal.mjs";

const DEFAULT_ANSWERS = {
  chooseActive: (value) => ({ activeCharacterId: value.candidateIds[0] }),
  rerollDice: () => ({ diceToReroll: [] }),
  switchHands: () => ({ removedHandIds: [] }),
  selectCard: (value) => ({ selectedDefinitionId: value.candidateDefinitionIds[0] }),
};

function snapshot(state) {
  return state.players.map((player) => player.characters.map((character) => ({
    health: character.variables.health,
    defeated: !character.variables.alive,
  })));
}

function change(before, after) {
  return before.map((characters, seat) => characters.map((character, index) => ({
    health: after[seat][index].health - character.health,
    defeated: after[seat][index].defeated && !character.defeated,
  })));
}

export function endOfRound(state, who, seed, opponentCards = null) {
  const start = redealHiddenCards(state, who, seed, opponentCards);
  const before = snapshot(start);
  const round = start.roundNumber;
  const game = new Game(start, {});
  engineWork.endOfRound.copies += 1;
  return new Promise((resolve) => {
    let done = false;
    const finish = (result) => {
      if (done) return;
      done = true;
      resolve(result);
      game.terminate();
    };
    for (const player of [0, 1]) {
      game.players[player].io = {
        notify: null,
        rpc: ({ request }) => {
          if (game.state.roundNumber > round) {
            finish({ change: change(before, snapshot(game.state)) });
            return new Promise(() => {});
          }
          if (request.$case === "action") {
            const end = request.value.action.findIndex((action) => action.action?.$case === "declareEnd" && (action.validity ?? 0) === 0);
            const index = end === -1 ? request.value.action.findIndex((action) => (action.validity ?? 0) === 0) : end;
            const chosen = request.value.action[index];
            return Promise.resolve({ response: { $case: "action", value: { chosenActionIndex: index, usedDice: chosen.autoSelectedDice } } });
          }
          return Promise.resolve({ response: { $case: request.$case, value: DEFAULT_ANSWERS[request.$case](request.value) } });
        },
      };
    }
    game.start().then(
      (winner) => finish({ change: change(before, snapshot(game.state)), ended: true, winner }),
      () => finish({ change: change(before, snapshot(game.state)), ended: true, winner: null }),
    );
  });
}
