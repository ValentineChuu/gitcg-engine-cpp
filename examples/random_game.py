"""Play one game between two random players through the worker: the whole protocol in one small script.

Build the worker first (make -j8 build/release/worker), then run from anywhere:

    python3 examples/random_game.py [seed]

Each decision names its request type and lists the choices; this player picks a random legal one. A bot would
read the decision's state (that player's own view of the board) and choose instead.
"""

import json
import random
import subprocess
import sys
from pathlib import Path

WORKER = Path(__file__).resolve().parents[1] / "build" / "release" / "worker"

# Two decks from the community's weekly tournaments: 3 character ids and 30 action-card ids each.
DECKS = [
    {"characters": [1205, 2501, 1513],
     "cards": [215131, 311105, 312004, 321038, 321038, 330012, 331010, 331501, 331501, 331502, 332004, 332004,
               332019, 332019, 332022, 332022, 332029, 332029, 332032, 332032, 332041, 332041, 332051, 332052,
               332054, 332054, 332057, 332057, 332064, 332064]},
    {"characters": [1711, 1210, 1217],
     "cards": [212171, 217111, 311105, 311106, 311106, 312017, 312027, 312027, 312031, 312031, 330013, 331006,
               331202, 331721, 331721, 331807, 331807, 332001, 332001, 332004, 332004, 332006, 332006, 332017,
               332017, 332020, 332032, 332032, 333015, 333015]},
]


def random_response(decision: dict, choose: random.Random) -> dict:
    """A legal answer to any request: an action, a new active character, a card, dice to reroll, cards to swap."""
    request, me = decision["request"], decision["state"]["player"][decision["who"]]
    kind, value = request["$case"], request["value"]
    if kind == "action":
        valid = [index for index, action in enumerate(value["action"]) if action.get("validity", 0) == 0]
        index = choose.choice(valid)
        # autoSelectedDice is a legal payment the engine already worked out for this action.
        return {"$case": "action", "value": {"chosenActionIndex": index, "usedDice": value["action"][index].get("autoSelectedDice", [])}}
    if kind == "chooseActive":
        return {"$case": "chooseActive", "value": {"activeCharacterId": choose.choice(value["candidateIds"])}}
    if kind == "selectCard":
        return {"$case": "selectCard", "value": {"selectedDefinitionId": choose.choice(value["candidateDefinitionIds"])}}
    if kind == "rerollDice":
        return {"$case": "rerollDice", "value": {"diceToReroll": [die for die in me["dice"] if choose.random() < 0.5]}}
    if kind == "switchHands":
        return {"$case": "switchHands", "value": {"removedHandIds": [card["id"] for card in me["handCard"] if choose.random() < 0.5]}}
    raise ValueError(f"unknown request {kind}")


def main() -> None:
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 1
    choose = random.Random(seed)
    worker = subprocess.Popen([str(WORKER)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)

    def send(message: dict) -> None:
        worker.stdin.write(json.dumps(message) + "\n")
        worker.stdin.flush()

    send({"type": "create", "game": 1, "decks": DECKS, "seed": seed})
    decisions = 0
    for line in worker.stdout:
        message = json.loads(line)
        if message["type"] == "decision":
            decisions += 1
            send({"type": "respond", "game": message["game"], "who": message["who"],
                  "response": random_response(message, choose)})
        elif message["type"] == "finished":
            winner = "a draw" if message["winner"] is None else f"player {message['winner']} won"
            print(f"{winner} in round {message['rounds']} after {decisions} decisions; "
                  f"characters lost per player: {message['defeated']}, damage dealt: {message['damageDealt']}")
            break
        elif message["type"] == "failed":
            print(f"the game failed: {message['message']}")
            break
    send({"type": "shutdown"})
    worker.wait()


if __name__ == "__main__":
    main()
