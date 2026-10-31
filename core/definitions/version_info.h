// Which game versions a definition belongs to (core/src/base/version.ts).
#pragma once

#include <string>

namespace gitcg {

struct VersionInfo {
  bool isSince = true;              // "since" or "until"
  std::string version = "v3.3.0";  // DEFAULT_VERSION_INFO is since v3.3.0
};

// Index of a version in the core's VERSIONS list; -1 when unknown.
int versionIndex(const std::string& version);

inline bool versionLessThan(const std::string& first, const std::string& second) {
  return versionIndex(first) < versionIndex(second);
}

}  // namespace gitcg
