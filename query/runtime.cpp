// Query set operations and traversal, ported from Piovium query/runtime.ts (AGPL-3.0).
#include "query/runtime.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_map>

#include "core/definitions/game_data.h"
#include "core/rules/entity_lookup.h"
#include "query/numerical.h"
#include "query/ordering.h"
#include "query/card_cost.h"

namespace gitcg {
namespace {
using Entries = std::vector<int>;
bool contains(const Entries& values, int value) {
  return std::find(values.begin(), values.end(), value) != values.end();
}
void add(Entries& values, int value) { if (!contains(values, value)) values.push_back(value); }
std::string typeOf(const AnyState& state) {
  if (state.isCharacter()) return "character";
  switch (state.entity()->definition->type) {
    case EntityType::EventCard: return "eventCard";
    case EntityType::Status: return "status";
    case EntityType::CombatStatus: return "combatStatus";
    case EntityType::Equipment: return "equipment";
    case EntityType::Support: return "support";
    case EntityType::Summon: return "summon";
    case EntityType::Attachment: return "attachment";
  }
  throw std::runtime_error("Invalid entity type");
}
std::string areaOf(AreaType area) {
  switch (area) {
    case AreaType::Characters: return "characters";
    case AreaType::CombatStatuses: return "combatStatuses";
    case AreaType::Summons: return "summons";
    case AreaType::Supports: return "supports";
    case AreaType::Hands: return "hands";
    case AreaType::Pile: return "pile";
    case AreaType::RemovedEntities: return "removedEntities";
  }
  throw std::runtime_error("Invalid area");
}
const std::vector<Name>& tagsOf(const AnyState& state) {
  return state.isCharacter() ? state.character()->definition->tags : state.entity()->definition->tags;
}
int definitionOf(const AnyState& state) {
  return state.isCharacter() ? state.character()->definition->id : state.entity()->definition->id;
}

class Runner {
 public:
  Runner(const GameState& state, int who, const QueryExpression& query)
      : state_(state), who_(who), query_(query), entries_(getAllEntities(state)) {
    diceCosts_.resize(entries_.size()); initialPileFlags_.resize(entries_.size());
    for (size_t index = 0; index < entries_.size(); ++index) {
      int i = static_cast<int>(index);
      const auto& entry = entries_[i];
      byId_[entry.state.id()] = i;
      if (entry.state.isCharacter()) characters_.push_back(i);
      else if (entry.area.type == AreaType::Characters) entitiesOnCharacters_.push_back(i);
      if (offStage(i)) attachables_.push_back(i);
      if (typeOf(entry.state) == "attachment") attachments_.push_back(i);
    }
    // Map.set replaces a duplicate value without moving its key.
    for (size_t i = 0; i < entries_.size(); ++i) add(all_, byId_.at(entries_[i].state.id()));
  }

  std::vector<AnyState> execute() {
    const auto& expression = query_.expression.asArray();
    Entries result;
    if (expression.at(0).asString() == "orderBy") {
      result = evaluate(expression.at(1), all_);
      const auto& specifications = expression.at(2).asArray();
      query::sortLikeJavaScript(result, [&](int a, int b) {
        std::vector<double> leftValues, rightValues;
        for (const auto& spec : specifications) leftValues.push_back(specification(spec, a));
        for (const auto& spec : specifications) rightValues.push_back(specification(spec, b));
        for (size_t i = 0; i < specifications.size(); ++i) {
          double left = leftValues[i], right = rightValues[i];
          if (left < right) return true;
          if (left > right) return false;
        }
        return a < b;
      });
      // JSON.stringify serializes the builder's unbounded (+Infinity) limit as null.
      double limit = expression.at(3).isNull() ? std::numeric_limits<double>::infinity() : expression.at(3).asNumber();
      double end = std::isnan(limit) ? 0 : std::trunc(limit);
      if (end < 0) end = std::max(0.0, static_cast<double>(result.size()) + end);
      if (end < result.size()) result.resize(static_cast<size_t>(end));
    } else {
      result = evaluate(query_.expression, all_);
      std::sort(result.begin(), result.end());
    }
    std::vector<AnyState> states;
    for (int index : result) states.push_back(entries_[index].state);
    return states;
  }

 private:
  bool offStage(int index) const {
    auto type = entries_[index].area.type;
    return type == AreaType::Hands || type == AreaType::Pile;
  }
  double variable(int index, const std::string& key) const {
    const auto& entry = entries_[index];
    const auto& entity = entry.state;
    auto type = typeOf(entity);
    bool actionCard = type == "equipment" || type == "support" || type == "eventCard";
    if (actionCard && key == query::inInitialPileKey) {
      if (initialPileFlags_[index]) return *initialPileFlags_[index];
      for (auto* definition : state_.players[entry.area.who]->initialPile) {
        if (definition->id == definitionOf(entity)) { initialPileFlags_[index] = 1; return 1; }
      }
      initialPileFlags_[index] = 0;
      return 0;
    }
    if (actionCard && key == query::diceCostKey) {
      if (!diceCosts_[index]) diceCosts_[index] = query::cardDiceCost(state_, entity.entity());
      return *diceCosts_[index];
    }
    return entity.variables().valueOr(intern(key), query::undefinedVariable());
  }
  double specification(const JsonValue& spec, int index) const {
    return query::evaluateSpecification(spec, [&](const std::string& key) { return variable(index, key); }, query_);
  }
  int position(int index) const {
    const auto& entry = entries_[index];
    const auto& characters = state_.players[entry.area.who]->characters;
    for (size_t i = 0; i < characters.size(); ++i) if (characters[i]->id == entry.state.id()) return i;
    throw std::runtime_error("Invalid character index");
  }
  bool satisfiesPosition(int index, const std::string& spec) const {
    const auto& entry = entries_[index];
    if (!entry.state.isCharacter()) return false;
    const auto& player = *state_.players[entry.area.who];
    int active = activeCharacterIndex(player);
    bool isActive = player.activeCharacterId == entry.state.id();
    if (spec == "active") return isActive;
    if (spec == "standby") return !isActive;
    if (spec != "next" && spec != "prev") throw std::runtime_error("Invalid position " + spec);
    if (isActive) return false;
    int size = player.characters.size(), direction = spec == "next" ? 1 : -1;
    for (int step = 1; step <= size; ++step) {
      int candidate = (active + direction * step + size) % size;
      if (query::truthy(player.characters[candidate]->variables.valueOr(intern("alive"), 0))) {
        return player.characters[candidate]->id == entry.state.id();
      }
    }
    throw std::runtime_error("Position next/prev has no living character (JS would not terminate)");
  }
  Entries evaluate(const JsonValue& value, const Entries& universe) {
    const auto& expr = value.asArray();
    const auto& op = expr.at(0).asString();
    auto operand = [&](size_t i, const Entries& candidates) { return evaluate(expr.at(i), candidates); };
    if (op == "id") {
      double id = expr.at(1).asNumber();
      if (!std::isfinite(id) || std::trunc(id) != id || id < std::numeric_limits<int>::min()
          || id > std::numeric_limits<int>::max()) return {};
      auto found = byId_.find(static_cast<int>(id));
      return found == byId_.end() ? Entries{} : Entries{found->second};
    }
    if (op == "intersection") {
      Entries result = universe;
      for (size_t i = 1; i < expr.size(); ++i) {
        auto right = operand(i, result);
        Entries next;
        const auto& smaller = result.size() <= right.size() ? result : right;
        for (int entry : smaller) if (contains(result, entry) && contains(right, entry)) add(next, entry);
        result = std::move(next);
      }
      return result;
    }
    if (op == "union") {
      Entries result;
      for (size_t i = 1; i < expr.size(); ++i) for (int entry : operand(i, universe)) add(result, entry);
      return result;
    }
    if (op == "orElse") {
      auto result = operand(1, all_);
      return result.empty() ? operand(2, all_) : result;
    }
    if (op == "not" || op == "exclude") {
      Entries left = op == "not" ? universe : operand(1, all_);
      Entries right = op == "not" ? operand(1, all_) : operand(2, left);
      Entries result;
      for (int entry : left) if (!contains(right, entry)) result.push_back(entry);
      return result;
    }
    if (op == "recentOppFrom") {
      Entries result;
      for (int base : operand(1, characters_)) {
        if (!entries_[base].state.isCharacter()) throw std::runtime_error("recentOppFrom requires characters");
        int targetWho = 1 - entries_[base].area.who;
        Entries targets;
        for (int index : characters_) if (entries_[index].area.who == targetWho) targets.push_back(index);
        std::stable_sort(targets.begin(), targets.end(), [&](int a, int b) { return position(a) < position(b); });
        if (targets.empty()) continue;
        int chosen = -1;
        double best = std::numeric_limits<double>::infinity();
        // The first character's virtual trailing position is inserted BEFORE the physical first position.
        auto consider = [&](int index, int targetPosition) {
          if (variable(index, "alive") == 0) return;
          double distance = std::abs(targetPosition - position(base));
          if (distance < best) { best = distance; chosen = index; }
        };
        consider(targets.front(), targets.size());
        for (int index : targets) consider(index, position(index));
        if (chosen >= 0) add(result, chosen);
      }
      return result;
    }
    Entries related;
    if (op == "has") related = operand(1, entitiesOnCharacters_);
    if (op == "at") related = operand(1, characters_);
    if (op == "on") related = operand(1, attachables_);
    if (op == "with") related = operand(1, attachments_);
    std::vector<Name> selectedTags;
    if (op == "tagOf") {
      const std::vector<std::string> category = expr.at(1).asString() == "weapon"
          ? std::vector<std::string>{"sword", "claymore", "pole", "catalyst", "bow"}
          : std::vector<std::string>{"cryo", "hydro", "pyro", "electro", "anemo", "geo", "dendro"};
      for (int base : operand(2, characters_)) for (Name tag : tagsOf(entries_[base].state)) {
        if (std::find(category.begin(), category.end(), nameText(tag)) != category.end()) selectedTags.push_back(tag);
      }
    }
    auto relatedId = [&](int id) {
      return std::any_of(related.begin(), related.end(), [&](int index) { return entries_[index].state.id() == id; });
    };
    Entries result;
    for (int index : universe) {
      const auto& entry = entries_[index];
      auto type = typeOf(entry.state);
      bool matches = false;
      if (op == "area") {
        matches = areaOf(entry.area.type) == expr.at(1).asString();
        if (expr.at(2).asString() == "true") {
          if (entry.area.type == AreaType::Characters) matches &= entry.state.isCharacter();
          if (offStage(index)) matches &= type != "attachment";
        }
      } else if (op == "defeated") matches = (variable(index, "alive") == 0) == (expr.at(1).asString() == "only");
      else if (op == "definition") matches = definitionOf(entry.state) == expr.at(1).asNumber();
      else if (op == "offStage") matches = offStage(index);
      else if (op == "onStage") matches = !offStage(index);
      else if (op == "position") matches = satisfiesPosition(index, expr.at(1).asString());
      else if (op == "type") matches = type == expr.at(1).asString();
      else if (op == "who") matches = entry.area.who == (expr.at(1).asString() == "my" ? who_ : 1 - who_);
      else if (op == "tag") {
        const auto& tags = tagsOf(entry.state);
        matches = std::find(tags.begin(), tags.end(), intern(expr.at(1).asString())) != tags.end();
      } else if (op == "tagOf") {
        for (Name tag : tagsOf(entry.state)) if (std::find(selectedTags.begin(), selectedTags.end(), tag) != selectedTags.end()) matches = true;
      } else if (op == "variables") matches = query::truthy(specification(expr.at(1), index));
      else if (op == "at") matches = entry.area.type == AreaType::Characters && !entry.state.isCharacter() && relatedId(entry.area.characterId);
      else if (op == "on") matches = offStage(index) && type == "attachment" && relatedId(entry.area.cardId);
      else if (op == "has") {
        if (entry.state.isCharacter()) for (const auto& child : entry.state.character()->entities) if (relatedId(child->id)) matches = true;
      } else if (op == "with") {
        if (entry.state.isEntity()) for (const auto& child : entry.state.entity()->attachments) if (relatedId(child->id)) matches = true;
      } else throw std::runtime_error("Unknown query operator " + op);
      if (matches) result.push_back(index);
    }
    return result;
  }

  const GameState& state_;
  int who_;
  const QueryExpression& query_;
  std::vector<EntityWithArea> entries_;
  std::unordered_map<int, int> byId_;
  Entries all_, characters_, entitiesOnCharacters_, attachables_, attachments_;
  mutable std::vector<std::optional<double>> diceCosts_, initialPileFlags_;
};
}  // namespace

std::vector<AnyState> runQuery(const GameState& state, int who, const QueryExpression& query) {
  if (who != 0 && who != 1) throw std::runtime_error("Query player must be 0 or 1");
  return Runner(state, who, query).execute();
}
}  // namespace gitcg
