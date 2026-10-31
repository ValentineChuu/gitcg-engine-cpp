// Interned strings for variable names and tags: card data compares and stores them often, so each distinct
// string gets a small number once, and the number is used everywhere else.
#pragma once

#include <string>
#include <string_view>

namespace gitcg {

using Name = int;

Name intern(std::string_view text);
const std::string& nameText(Name name);

}  // namespace gitcg
