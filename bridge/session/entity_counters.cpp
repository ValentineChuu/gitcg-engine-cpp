#include "bridge/session/entity_counters.h"

#include <string>

namespace gitcg {

JsonValue boardCountersById(const GameStatePointer& state) {
  // Keep in sync with DISPLAYED_COUNTERS in entity_counters.mjs.
  static const Name DISPLAYED[] = {intern("usage"),    intern("usagePerRound"), intern("duration"),
                                   intern("shield"),   intern("progress"),      intern("layer"),
                                   intern("nightsoul"), intern("count"),        intern("barrierUsage")};
  JsonValue result = JsonValue::object();
  auto collect = [&](const EntityPointer& entity) {
    JsonValue counters = JsonValue::object();
    for (Name name : DISPLAYED) {
      if (auto value = entity->variables.find(name)) counters[nameText(name)] = *value;
    }
    // Entity ids are negative, so JS keeps these keys in insertion order.
    result[std::to_string(entity->id)] = std::move(counters);
  };
  for (const PlayerPointer& player : state->players) {
    for (const CharacterPointer& character : player->characters) {
      for (const EntityPointer& entity : character->entities) collect(entity);
    }
    for (const EntityPointer& entity : player->combatStatuses) collect(entity);
    for (const EntityPointer& entity : player->summons) collect(entity);
    for (const EntityPointer& entity : player->supports) collect(entity);
  }
  return result;
}

}  // namespace gitcg
