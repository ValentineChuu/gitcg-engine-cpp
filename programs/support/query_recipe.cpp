// Mechanical translation of the test recipe's real-$ calls to the public C++ builder.
#include "programs/support/query_recipe.h"

#include <stdexcept>
#include "query/function.h"

namespace gitcg::testing {
namespace {
JsonValue argument(const JsonValue& value) {
  if (value.isObject() && value.find("key")) {
    return value.at("key").asString() == "diceCost" ? query::diceCostKey : query::inInitialPileKey;
  }
  return value;
}
query::Query macro(const std::string& name) {
  auto macros = query::dollar().macros();
#define MACRO(name) if (nameText == #name) return macros.name();
  const auto& nameText = name;
  MACRO(myActive) MACRO(oppActive) MACRO(myFirstEnergyNotFull) MACRO(oppActivePrioritized)
  MACRO(myMinHealth) MACRO(oppMinHealth) MACRO(myMaxHealth) MACRO(oppMaxHealth)
  MACRO(myMostInjured) MACRO(oppMostInjured) MACRO(myLeastInjured) MACRO(oppLeastInjured)
  MACRO(myHandsOrderByCost) MACRO(oppHandsOrderByCost) MACRO(myHandsNotFree) MACRO(oppHandsNotFree)
  MACRO(myPileNotFree) MACRO(oppPileNotFree)
#undef MACRO
  throw std::runtime_error("Unknown macro " + name);
}
query::Query unary(const query::Query& value, const std::string& name, const JsonValue::Array& step) {
  bool call = step.size() > 1;
  auto operand = call ? buildQueryRecipe(step[1].at("query")) : query::Query();
#define UNARY(js, cpp) if (name == js) return call ? value.cpp(operand) : value.cpp();
  UNARY("not", not_) UNARY("has", has) UNARY("at", at) UNARY("with", with) UNARY("on", on) UNARY("recentOppFrom", recentOppFrom)
#undef UNARY
  throw std::runtime_error("Unknown unary operator");
}
}  // namespace
query::Query buildQueryRecipe(const JsonValue& recipe) {
  auto result = query::dollar();
  for (const auto& entry : recipe.at("steps").asArray()) {
    const auto& step = entry.asArray();
    const auto& name = step.at(0).asString();
#define GETTER(method) if (name == #method) { result = result.method(); continue; }
    GETTER(any) GETTER(my) GETTER(opp) GETTER(onStage) GETTER(offStage) GETTER(character) GETTER(combatStatus)
    GETTER(summon) GETTER(support) GETTER(attachment) GETTER(hand) GETTER(pile) GETTER(vCharacter) GETTER(vHand) GETTER(vPile)
    GETTER(typeEquipment) GETTER(typeSupport) GETTER(typeStatus) GETTER(typeEventCard)
    GETTER(equipped) GETTER(equipment) GETTER(status) GETTER(eventCard)
    GETTER(active) GETTER(prev) GETTER(next) GETTER(standby) GETTER(onlyDefeated) GETTER(includesDefeated) GETTER(notInitial)
#undef GETTER
    if (name == "macro") result = macro(step.at(1).asString());
    else if (name == "id") result = result.id(step.at(1).asNumber());
    else if (name == "def") result = result.def(step.at(1).asNumber());
    else if (name == "tag") {
      std::vector<std::string> tags;
      for (size_t i = 1; i < step.size(); ++i) tags.push_back(step[i].asString());
      result = result.tag(tags);
    } else if (name == "tagOf") result = result.tagOf(step.at(1).asString(), buildQueryRecipe(step.at(2).at("query")));
    else if (name == "limit") result = result.limit(step.at(1).asNumber());
    else if (name == "orderBy") result = step.size() == 2 ? result.orderBy(argument(step[1]))
        : result.orderBy(argument(step[1]), step[2].asString(), argument(step[3]));
    else if (name == "orderByRaw") result = result.orderByRaw({step.begin() + 1, step.end()});
    else if (name == "orderByFn") {
      auto code = step[1].at("function").asString();
      result = result.orderByFn([code](const QueryVariables& v) { return query::evaluateFunction(code, v); });
    } else if (name == "var" || name == "cost") {
      size_t offset = name == "cost" ? 0 : 1;
      JsonValue key = name == "cost" ? JsonValue(query::diceCostKey) : argument(step[1]);
      if (step[1].isObject() && step[1].find("function") && name == "var") {
        auto code = step[1].at("function").asString();
        result = result.var([code](const QueryVariables& v) { return query::evaluateFunction(code, v); });
      } else if (step.at(offset + 1).isObject() && step[offset + 1].find("function")) {
        auto code = step[offset + 1].at("function").asString();
        result = result.var(key, [code](double value) { return query::evaluateScalarFunction(code, value); });
      } else if (step.size() == offset + 2) result = result.var(key, step[offset + 1].asNumber());
      else result = result.var(key, step[offset + 1].asString(), argument(step[offset + 2]));
    } else if (name == "union" || name == "intersection") {
      std::vector<query::Query> operands;
      for (size_t i = 1; i < step.size(); ++i) operands.push_back(buildQueryRecipe(step[i].at("query")));
      result = name == "union" ? result.union_(operands) : result.intersection(operands);
    } else if (name == "orElse" || name == "exclude") {
      auto right = buildQueryRecipe(step.at(1).at("query"));
      result = name == "orElse" ? result.orElse(right) : result.exclude(right);
    } else result = unary(result, name, step);
  }
  return result;
}
bool recipeHasFunction(const JsonValue& value) {
  if (value.isObject()) {
    if (value.find("function")) return true;
    for (const auto& [key, item] : value.asObject()) if (recipeHasFunction(item)) return true;
  } else if (value.isArray()) for (const auto& item : value.asArray()) if (recipeHasFunction(item)) return true;
  return false;
}
}  // namespace gitcg::testing
