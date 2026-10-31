#include "core/values/names.h"

#include <deque>
#include <unordered_map>

namespace gitcg {

namespace {

struct NameTable {
  std::deque<std::string> texts;
  std::unordered_map<std::string_view, Name> numbers;
};

NameTable& table() {
  static NameTable instance;
  return instance;
}

}  // namespace

Name intern(std::string_view text) {
  NameTable& names = table();
  auto found = names.numbers.find(text);
  if (found != names.numbers.end()) return found->second;
  names.texts.emplace_back(text);
  Name number = static_cast<Name>(names.texts.size() - 1);
  names.numbers.emplace(names.texts.back(), number);
  return number;
}

const std::string& nameText(Name name) {
  return table().texts.at(name);
}

}  // namespace gitcg
