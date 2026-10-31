"""Deterministic stage opponents and legal decks drawn from simulated account collections."""
import json
import pathlib
import random

from autogitcg.deck_building.random_deck_builder import random_deck
from autogitcg.progression.stage_catalog import build_stage_catalog
from autogitcg.training.collection_pool import simulate_collections

stages, unsupported = build_stage_catalog()
collections = simulate_collections(stages, 1, 4, 702)
randomizer = random.Random(703)
entries = []
for index, stage in enumerate(stages):
    collection = collections[index % len(collections)]
    deck = random_deck(list(collection.characters), collection.action_cards, randomizer)
    entries.append({
        'levelId': stage.level_id, 'kind': stage.kind, 'name': stage.name,
        'decks': [{'characters': list(deck.characters), 'cards': list(deck.cards)},
                  {'characters': list(stage.enemy_deck.characters), 'cards': list(stage.enemy_deck.cards)}],
        'equipment': [item.to_message() for item in stage.enemy_equipment],
        'collection': index % len(collections),
    })
output = pathlib.Path(__file__).resolve().parent / 'generated/coverage_stages.json'
output.write_text(json.dumps({'stages': entries, 'unsupported': len(unsupported), 'collections': [
    {'characters': list(collection.characters), 'cards': dict(collection.action_cards),
     'matchesPlayed': collection.matches_played} for collection in collections
]}, indent=2) + '\n')
print(f'{len(entries)} stages, {len(unsupported)} unsupported, {len(collections)} collections -> {output}')
