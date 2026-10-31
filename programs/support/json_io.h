// Ordered JSON input/output for differential fixtures; independent of a protocol library.
#pragma once

#include <string_view>
#include "core/json/json_value.h"

namespace gitcg::testing {
JsonValue parseJson(std::string_view input);
JsonValue readJson(const std::string& path);
std::string stringifyJson(const JsonValue& value);
}  // namespace gitcg::testing
