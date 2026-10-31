#include "bridge/search/turn_line_rules.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <unistd.h>
#include "core/definitions/entity_definition.h"
#include "core/json/json_text.h"

namespace gitcg {
namespace {
const JsonValue& facts() {
  static const JsonValue value = [] {
    // data/card_line_facts.json in the engine (made by reference/tools/export_card_facts.mjs), found from the worker's
    // own place (build/<mode>/worker); GITCG_CARD_FACTS overrides it.
    std::vector<std::filesystem::path> paths;
    if (const char* path = std::getenv("GITCG_CARD_FACTS")) paths.emplace_back(path);
    std::error_code error;
    auto executable = std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error) paths.push_back(executable.parent_path() / "../../data/card_line_facts.json");
    paths.emplace_back("data/card_line_facts.json");
    for (const auto& path : paths) {
      std::ifstream input(path);
      if (!input) continue;
      std::stringstream text; text << input.rdbuf(); return parseJson(text.str());
    }
    throw std::runtime_error("data/card_line_facts.json is missing; set GITCG_CARD_FACTS");
  }();
  return value;
}
const JsonValue* cardFacts(const ActionInfo& action) {
  return action.type == ActionType::PlayCard ? facts().find(std::to_string(action.skill->caller.entity()->definition->id)) : nullptr;
}
bool flag(const JsonValue* facts, const char* key) {
  const JsonValue* value = facts ? facts->find(key) : nullptr;
  return value && value->isBool() && value->asBool();
}
}  // namespace
std::string cardOutcomeKey(const ActionInfo& action) {
  std::string key = std::to_string(action.skill->caller.entity()->definition->id) + ":[";
  for (size_t i = 0; i < action.targets.size(); ++i) { if (i) key += ','; key += std::to_string(action.targets[i].id()); }
  key += "]:";
  for (int die : action.autoSelectedDice) key += std::to_string(die) + ',';
  return key;
}
bool endsLine(const ActionInfo& action) {
  const auto* f = cardFacts(action); return flag(f, "draws") || flag(f, "random");
}
bool cardIsLeftOutOfReplies(const ActionInfo& action) {
  const auto* f = cardFacts(action); return flag(f, "draws") || flag(f, "random") || flag(f, "givesDice");
}
std::vector<int> branchingMoves(const std::vector<ActionInfo>& actions, const GameStatePointer& state) {
  std::vector<int> valid;
  std::set<int> playable;
  for (size_t i = 0; i < actions.size(); ++i) {
    if (actions[i].validity != 0 || actions[i].willBeEffectless) continue;
    valid.push_back(i);
    if (actions[i].type == ActionType::PlayCard) playable.insert(actions[i].skill->caller.id());
  }
  std::map<int, int> tuneByDie;
  for (int i : valid) {
    const auto& action = actions[i];
    if (action.type != ActionType::ElementalTuning) continue;
    auto current = tuneByDie.find(action.result);
    if (current == tuneByDie.end() || (playable.contains(actions[current->second].card->id) && !playable.contains(action.card->id)))
      tuneByDie[action.result] = i;
  }
  std::vector<int> kept;
  std::set<std::string> seen;
  for (int i : valid) {
    const auto& action = actions[i];
    if (action.type == ActionType::ElementalTuning && tuneByDie.at(action.result) != i) continue;
    if (action.type == ActionType::PlayCard) {
      const auto* f = cardFacts(action);
      if (flag(f, "food") && !action.targets.empty()) {
        int targetId = action.targets[0].id();
        bool cut = false;
        for (const auto& side : state->players) for (const auto& character : side->characters) {
          if (character->id != targetId) continue;
          for (const auto& entity : character->entities) if (entity->definition->id == 303300) cut = true;
          if (flag(f, "heals") && character->variables.valueOr(intern("health"), 0) >= character->variables.valueOr(intern("maxHealth"), 0)) cut = true;
        }
        if (cut) continue;
      }
      if (!seen.insert(cardOutcomeKey(action)).second) continue;
    }
    kept.push_back(i);
  }
  return kept;
}
}  // namespace gitcg
