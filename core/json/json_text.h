// JSON text in and out. Numbers are written as JSON.stringify writes them (integers without a fraction, other
// values in shortest round-trip form, NaN and infinities as null), so messages match Node's byte for byte.
#pragma once

#include <string>
#include <string_view>

#include "core/json/json_value.h"

namespace gitcg {

std::string writeJson(const JsonValue& value);
void appendJson(std::string& out, const JsonValue& value);
void appendJsonNumber(std::string& out, double number);

JsonValue parseJson(std::string_view text);  // throws std::runtime_error on malformed input

}  // namespace gitcg
