"""Every deck source the equivalence tests cover, as one JSON file for the Node recorder (record_reference.mjs).

Each entry: {"origin": str, "characters": [...], "cards": [...], "equipment": [{who, characterIndex, definitionId}]}.
Equipment (Serious Showdown talents) is given for the NPC playing as player 1, as the stages define it.

Usage: .venv/bin/python tests/export_test_decks.py [--output tests/generated/test_decks.json]
"""

import argparse
import json
from pathlib import Path

from autogitcg.data.npc_decks import load_npc_decks
from autogitcg.data.tournament_decks import downloaded_versions, load_tournament_decks
from autogitcg.progression.stage_catalog import build_stage_catalog
from autogitcg.training.deck_pools import load_synergy_decks

DEFAULT_OUTPUT = Path(__file__).resolve().parent / "generated" / "test_decks.json"


def deck_entry(origin, deck, equipment=()):
    return {"origin": origin, "characters": list(deck.characters), "cards": list(deck.cards),
            "equipment": [item.to_message() for item in equipment]}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    arguments = parser.parse_args()

    entries = []
    # The engine only, not training: the locked test lineups are included too.
    for tournament_deck in load_tournament_decks(downloaded_versions()):
        entries.append(deck_entry(f"tournament:{tournament_deck.lineup_name}", tournament_deck.deck))
    for deck_id, deck in sorted(load_npc_decks().items()):
        entries.append(deck_entry(f"npc:{deck_id}", deck))
    stages, unsupported = build_stage_catalog()
    assert not unsupported, f"{len(unsupported)} unsupported stages"
    for stage in stages:
        entries.append(deck_entry(f"stage:{stage.level_id}:{stage.kind}", stage.enemy_deck, stage.enemy_equipment))
    synergy_decks, archetypes = load_synergy_decks()
    for deck, archetype in zip(synergy_decks, archetypes):
        entries.append(deck_entry(f"synergy:{archetype}", deck))

    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_text(json.dumps(entries))
    counts = {}
    for entry in entries:
        kind = entry["origin"].split(":")[0]
        counts[kind] = counts.get(kind, 0) + 1
    print(f"{len(entries)} decks -> {arguments.output}: {counts}")


if __name__ == "__main__":
    main()
