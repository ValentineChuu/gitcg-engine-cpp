#include "core/definitions/version_info.h"

#include <array>
#include <string_view>

namespace gitcg {

namespace {

constexpr std::array<std::string_view, 35> VERSIONS = {
    "v3.3.0", "v3.4.0", "v3.5.0", "v3.6.0", "v3.7.0", "v3.8.0", "v4.0.0", "v4.1.0", "v4.2.0",
    "v4.3.0", "v4.4.0", "v4.5.0", "v4.6.0", "v4.6.1", "v4.7.0", "v4.8.0", "v5.0.0", "v5.1.0",
    "v5.2.0", "v5.3.0", "v5.4.0", "v5.5.0", "v5.6.0", "v5.7.0", "v5.8.0", "v6.0.0", "v6.1.0",
    "v6.2.0", "v6.3.0", "v6.4.0", "v6.5.0", "v6.6.0", "v6.7.0", "v7.0.0", "v7.1.0",
};

}  // namespace

int versionIndex(const std::string& version) {
  for (size_t index = 0; index < VERSIONS.size(); ++index) {
    if (VERSIONS[index] == version) return static_cast<int>(index);
  }
  return -1;
}

}  // namespace gitcg
