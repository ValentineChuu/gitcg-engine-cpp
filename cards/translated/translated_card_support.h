// What translated card code includes: the skill context, the query builder, and `js::` helpers that keep JS
// semantics explicit (truthiness, array methods, definition fields read through live handles).
#pragma once

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"
#include "core/events/action_event_args.h"
#include "core/events/damage_event_args.h"
#include "core/events/entity_event_args.h"
#include "core/events/request_args.h"
#include "core/rules/costs.h"
#include "core/rules/entity_lookup.h"
#include "core/skills/skill_context.h"
#include "definitions/attributes/lambda_slots.h"
#include "definitions/registry/registry.h"
#include "query/builder.h"
#include "query/macro.h"

namespace gitcg::js {

// --- truthiness ---
inline bool truthy(bool value) { return value; }
inline bool truthy(double value) { return value != 0 && !std::isnan(value); }
inline bool truthy(const std::string& value) { return !value.empty(); }
inline bool truthy(const CharacterHandle&) { return true; }
inline bool truthy(const EntityHandle&) { return true; }
inline bool truthy(const AnyHandle&) { return true; }
// Present and itself truthy (undefined, 0, NaN and "" are all falsy in JS).
template <typename Value>
bool truthy(const std::optional<Value>& value) { return value.has_value() && truthy(*value); }

// --- numbers ---
template <typename... Values>
double math_min(double first, Values... rest) { double result = first; ((result = std::min<double>(result, rest)), ...); return result; }
template <typename... Values>
double math_max(double first, Values... rest) { double result = first; ((result = std::max<double>(result, rest)), ...); return result; }
inline double math_floor(double value) { return std::floor(value); }
inline double math_ceil(double value) { return std::ceil(value); }
inline double math_abs(double value) { return std::abs(value); }

// --- handles ---
inline AnyHandle handle(SkillContext& c, const AnyHandle& value) { return value; }
inline AnyHandle handle(SkillContext& c, const CharacterHandle& value) { return AnyHandle(&c, value.id(), true); }
inline AnyHandle handle(SkillContext& c, const EntityHandle& value) { return AnyHandle(&c, value.id(), false); }
inline AnyHandle handle(SkillContext& c, const AnyState& value) { return c.get(value); }
template <typename Value>
AnyHandle handle(SkillContext& c, const std::optional<Value>& value) { return handle(c, *value); }
inline EntityHandle entityHandle(SkillContext& c, const EntityHandle& value) { return value; }
inline EntityHandle entityHandle(SkillContext& c, const AnyHandle& value) { return value.asEntity(); }
inline EntityHandle entityHandle(SkillContext& c, const AnyState& value) { return EntityHandle(&c, value.id()); }
template <typename Value>
EntityHandle entityHandle(SkillContext& c, const std::optional<Value>& value) { return entityHandle(c, *value); }

template <typename Value>
std::vector<AnyHandle> handles(SkillContext& c, const std::optional<Value>& value) {
  return value ? std::vector<AnyHandle>{handle(c, *value)} : std::vector<AnyHandle>{};
}
inline std::vector<AnyHandle> handles(SkillContext& c, const AnyHandle& value) { return {value}; }
template <typename Value>
std::vector<AnyHandle> handles(SkillContext& c, const std::vector<Value>& values) {
  std::vector<AnyHandle> result;
  for (const Value& value : values) result.push_back(handle(c, value));
  return result;
}
inline std::vector<EntityHandle> entityHandles(SkillContext& c, const std::vector<EntityPointer>& entities) {
  std::vector<EntityHandle> result;
  for (const EntityPointer& entity : entities) result.emplace_back(&c, entity->id);
  return result;
}
inline std::vector<EntityHandle> entityHandles(SkillContext& c, std::initializer_list<EntityHandle> entities) { return entities; }
inline std::vector<EntityHandle> entityHandles(SkillContext&, const std::vector<EntityHandle>& entities) { return entities; }
inline std::vector<CharacterHandle> characterHandles(SkillContext& c, const std::vector<CharacterPointer>& characters) {
  std::vector<CharacterHandle> result;
  for (const CharacterPointer& character : characters) result.emplace_back(&c, character->id);
  return result;
}
inline std::vector<double> numbers(const std::vector<DiceType>& values) { return std::vector<double>(values.begin(), values.end()); }

// --- definitions read through handles (`x.definition.id`, `.type`, `.tags`) ---
struct DefinitionView {
  const CharacterDefinition* character = nullptr;
  const EntityDefinition* entity = nullptr;
};
inline DefinitionView definitionOf(const AnyHandle& value) {
  AnyState state = value.latest();
  return state.isCharacter() ? DefinitionView{state.character()->definition, nullptr} : DefinitionView{nullptr, state.entity()->definition};
}
inline double idOf(const DefinitionView& definition) { return definition.character ? definition.character->id : definition.entity->id; }
inline double idOf(const EntityDefinition& definition) { return definition.id; }
inline double idOf(const CharacterDefinition& definition) { return definition.id; }
inline double idOf(const EntityDefinition* definition) { return definition->id; }
std::string typeNameOf(const EntityDefinition& definition);
inline std::string typeNameOf(const CharacterDefinition&) { return "character"; }
inline std::string typeNameOf(const EntityDefinition* definition) { return typeNameOf(*definition); }
inline std::string typeNameOf(const DefinitionView& definition) {
  return definition.character ? std::string("character") : typeNameOf(*definition.entity);
}
inline const std::vector<Name>& tagsOf(const EntityDefinition& definition) { return definition.tags; }
inline const std::vector<Name>& tagsOf(const CharacterDefinition& definition) { return definition.tags; }
inline const std::vector<Name>& tagsOf(const EntityDefinition* definition) { return definition->tags; }
inline const std::vector<Name>& tagsOf(const DefinitionView& definition) {
  return definition.character ? definition.character->tags : definition.entity->tags;
}
inline bool hasTag(const std::vector<Name>& tags, const char* tag) {
  Name name = intern(tag);
  return std::find(tags.begin(), tags.end(), name) != tags.end();
}
// allCardDefinitions(filter) with a string: a card type name or else a tag.
inline bool cardMatches(const EntityDefinition& definition, const std::string& filter) {
  if (filter == "eventCard") return definition.type == EntityType::EventCard;
  if (filter == "support") return definition.type == EntityType::Support;
  if (filter == "equipment") return definition.type == EntityType::Equipment;
  return definition.hasTag(intern(filter));
}
inline std::string actionTypeName(ActionType type) {
  switch (type) {
    case ActionType::UseSkill: return "useSkill";
    case ActionType::PlayCard: return "playCard";
    case ActionType::SwitchActive: return "switchActive";
    case ActionType::ElementalTuning: return "elementalTuning";
    case ActionType::DeclareEnd: return "declareEnd";
  }
  return "";
}
inline EntityArea areaOf(const AnyHandle& state) { return state.isCharacter() ? state.asCharacter().area() : state.asEntity().area(); }
inline std::string areaTypeName(const EntityArea& area) {
  switch (area.type) {
    case AreaType::Characters: return "characters";
    case AreaType::CombatStatuses: return "combatStatuses";
    case AreaType::Summons: return "summons";
    case AreaType::Supports: return "supports";
    case AreaType::Hands: return "hands";
    case AreaType::Pile: return "pile";
    case AreaType::RemovedEntities: return "removedEntities";
  }
  return "";
}
inline bool isMine(const AnyHandle& value) {
  return value.isCharacter() ? value.asCharacter().isMine() : value.asEntity().isMine();
}

// --- arrays ---
template <typename Value>
double length(const std::vector<Value>& values) { return static_cast<double>(values.size()); }
template <typename Value>
Value index(const std::vector<Value>& values, double position) { return values.at(static_cast<size_t>(position)); }
template <typename Value, typename Fallback>
Value indexOr(const std::vector<Value>& values, double position, const Fallback& fallback) {
  size_t index = static_cast<size_t>(position);
  return position >= 0 && index < values.size() ? values[index] : Value(fallback);
}
template <typename Value>
std::optional<Value> at(const std::vector<Value>& values, double position) {
  long index = static_cast<long>(position);
  if (index < 0) index += static_cast<long>(values.size());
  if (index < 0 || index >= static_cast<long>(values.size())) return std::nullopt;
  return values[index];
}
template <typename Value, typename Item>
bool includes(const std::vector<Value>& values, const Item& item) { return std::find(values.begin(), values.end(), item) != values.end(); }
template <typename Value, typename Predicate>
std::vector<Value> filter(const std::vector<Value>& values, Predicate predicate) {
  std::vector<Value> result;
  for (const Value& value : values) {
    if (truthy(predicate(value))) result.push_back(value);
  }
  return result;
}
template <typename Value, typename Function>
auto map(const std::vector<Value>& values, Function function) {
  std::vector<decltype(function(values.front()))> result;
  for (const Value& value : values) result.push_back(function(value));
  return result;
}
template <typename Value, typename Predicate>
bool some(const std::vector<Value>& values, Predicate predicate) {
  return std::any_of(values.begin(), values.end(), [&](const Value& value) { return truthy(predicate(value)); });
}
template <typename Value, typename Predicate>
bool every(const std::vector<Value>& values, Predicate predicate) {
  return std::all_of(values.begin(), values.end(), [&](const Value& value) { return truthy(predicate(value)); });
}
template <typename Value, typename Predicate>
std::optional<Value> find(const std::vector<Value>& values, Predicate predicate) {
  for (const Value& value : values) {
    if (truthy(predicate(value))) return value;
  }
  return std::nullopt;
}

// A played card's fixed targets (states as they are now).
template <typename Value>
PlayCardTarget targetList(SkillContext& c, const std::vector<Value>& values) {
  PlayCardTarget target{PlayCardTarget::Rule::List, {}};
  for (const AnyHandle& value : handles(c, values)) target.targets.push_back(value.latest());
  return target;
}

// c.random(items): one random step even for an empty list, where JS reads items[NaN] = undefined.
template <typename Value>
std::optional<Value> randomOf(SkillContext& c, const std::vector<Value>& values) {
  int64_t step = c.stepRandom();
  if (values.empty()) return std::nullopt;
  return values[static_cast<size_t>(step % static_cast<int64_t>(values.size()))];
}

// Custom events by the definition VM's identity ("<file>:<offset>"); the registry is set once at startup.
void useCustomEventRegistry(const DefinitionRegistry* registry);
const CustomEvent* customEvent(const char* identity);

// --- typed query results ---
inline std::optional<CharacterHandle> optionalOfCharacters(const std::optional<AnyHandle>& value) {
  return value ? std::optional<CharacterHandle>(value->asCharacter()) : std::nullopt;
}
inline std::optional<EntityHandle> optionalOfEntities(const std::optional<AnyHandle>& value) {
  return value ? std::optional<EntityHandle>(value->asEntity()) : std::nullopt;
}
inline std::vector<CharacterHandle> listOfCharacters(const std::vector<AnyHandle>& values) {
  std::vector<CharacterHandle> result;
  for (const AnyHandle& value : values) result.push_back(value.asCharacter());
  return result;
}
inline std::vector<EntityHandle> listOfEntities(const std::vector<AnyHandle>& values) {
  std::vector<EntityHandle> result;
  for (const AnyHandle& value : values) result.push_back(value.asEntity());
  return result;
}
inline std::optional<CharacterHandle> optionalCharacter(SkillContext& c, const CharacterPointer& character) {
  return character ? std::optional<CharacterHandle>(CharacterHandle(&c, character->id)) : std::nullopt;
}

// --- optionals (`x?.member`) ---
template <typename Value, typename Function>
auto mapOptional(const std::optional<Value>& value, Function function) -> std::optional<decltype(function(*value))> {
  if (!value) return std::nullopt;
  return function(*value);
}
template <typename Value, typename Function>
auto flatMapOptional(const std::optional<Value>& value, Function function) -> decltype(function(*value)) {
  if (!value) return std::nullopt;
  return function(*value);
}
template <typename Value, typename Function>
void ifPresent(const std::optional<Value>& value, Function function) {
  if (value) function(*value);
}

// --- operators ---
inline double remainder(double left, double right) { return std::fmod(left, right); }  // JS %: sign of the dividend
template <typename Value, typename Fallback>
Value valueOr(const std::optional<Value>& value, const Fallback& fallback) { return value ? *value : Value(fallback); }
template <typename Left, typename Right>
bool strictEquals(const std::optional<Left>& left, const Right& right) { return left.has_value() && *left == right; }
template <typename Left, typename Right>
bool strictEquals(const Left& left, const std::optional<Right>& right) { return right.has_value() && left == *right; }
template <typename Value, typename Function, typename Initial>
Initial reduce(const std::vector<Value>& values, Function function, Initial accumulator) {
  for (const Value& value : values) accumulator = function(accumulator, value);
  return accumulator;
}
template <typename Value, typename Function>
void forEach(const std::vector<Value>& values, Function function) {
  for (const Value& value : values) function(value);
}
// array.slice(start, end): negative positions count from the end; both are clamped to the list.
template <typename Value>
std::vector<Value> slice(const std::vector<Value>& values, double start, double end = 1e300) {
  double size = static_cast<double>(values.size());
  auto clamp = [&](double position) { return position < 0 ? std::max(0.0, size + std::trunc(position)) : std::min(size, std::trunc(position)); };
  double from = clamp(start), to = clamp(end);
  if (to <= from) return {};
  return std::vector<Value>(values.begin() + static_cast<long>(from), values.begin() + static_cast<long>(to));
}
// [...a, b, ...c]: the parts joined in order.
template <typename Value>
std::vector<Value> concat(const std::vector<std::vector<Value>>& parts) {
  std::vector<Value> result;
  for (const auto& part : parts) result.insert(result.end(), part.begin(), part.end());
  return result;
}
// array.toSorted(compare): stable, like V8's TimSort with a consistent comparator.
template <typename Value, typename Compare>
std::vector<Value> toSorted(std::vector<Value> values, Compare compare) {
  std::stable_sort(values.begin(), values.end(), [&](const Value& left, const Value& right) { return compare(left, right) < 0; });
  return values;
}
// array.flatMap(f): f returns a list per item, concatenated in order.
template <typename Value, typename Function>
auto flatMap(const std::vector<Value>& values, Function function) {
  decltype(function(values.front())) result;
  for (const Value& value : values) {
    auto part = function(value);
    result.insert(result.end(), part.begin(), part.end());
  }
  return result;
}
std::vector<std::string> nameTexts(const std::vector<Name>& names);
double variableOf(const AnyHandle& state, Name name);
// new Set(values): distinct values in first-appearance order.
template <typename Value>
std::vector<Value> distinct(const std::vector<Value>& values) {
  std::vector<Value> result;
  for (const Value& value : values) {
    if (std::find(result.begin(), result.end(), value) == result.end()) result.push_back(value);
  }
  return result;
}

// --- results handed back to the definition VM ---
inline JsonValue toJson(const JsonValue& value) { return value; }
inline JsonValue toJson(bool value) { return JsonValue(value); }
inline JsonValue toJson(double value) { return JsonValue(value); }
inline JsonValue toJson(int value) { return JsonValue(value); }
inline JsonValue toJson(const std::string& value) { return JsonValue(value); }
inline JsonValue toJson(const CharacterHandle& value) { return JsonValue(value.id()); }
inline JsonValue toJson(const EntityHandle& value) { return JsonValue(value.id()); }
inline JsonValue toJson(const AnyHandle& value) { return JsonValue(value.id()); }
template <typename Value>
JsonValue toJson(const std::optional<Value>& value) { return value ? toJson(*value) : JsonValue(); }
template <typename Value>
JsonValue toJson(const std::vector<Value>& values) {
  JsonValue result = JsonValue::array();
  for (const Value& value : values) result.push(toJson(value));
  return result;
}
inline JsonValue ids(const std::vector<AnyHandle>& values) { return toJson(values); }
// Card or summon ids written as a list of handles (branded numbers).
inline std::vector<int> ids(const std::vector<double>& values) { return std::vector<int>(values.begin(), values.end()); }
inline std::vector<int> ids(const std::vector<const EntityDefinition*>& definitions) {
  std::vector<int> result;
  for (const EntityDefinition* definition : definitions) result.push_back(definition->id);
  return result;
}

// "top", "bottom", "random", "spaceAround", "topRange<N>", "topIndex<N>".
PileInsertion pileInsertion(const std::string& strategy);

}  // namespace gitcg::js
