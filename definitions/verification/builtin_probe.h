// Executable VM checks compared independently with the Node VM, from the shared attribute fixture.
#pragma once
#include <string_view>
#include "core/json/json_value.h"
namespace gitcg { JsonValue probeDefinitionBuiltins(std::string_view fixture); }
