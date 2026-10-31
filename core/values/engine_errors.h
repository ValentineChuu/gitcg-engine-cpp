// The core's error kinds (core/src/error.ts). A player's invalid answer loses them the game; data and internal
// errors fail the game; an aborted preview is caught by the previewer.
#pragma once

#include <stdexcept>
#include <string>

namespace gitcg {

class GameIoError : public std::runtime_error {
 public:
  GameIoError(int player, const std::string& message) : std::runtime_error(message), who(player) {}
  int who;
};

class GameDataError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class GameInternalError : public std::logic_error {
 public:
  using std::logic_error::logic_error;
};

// A player request where nobody can answer (a precalculation): ignored by the action pricing.
class IoNotProvidedError : public std::runtime_error {
 public:
  IoNotProvidedError() : std::runtime_error("io not provided") {}
};

class PreviewAbortedError : public std::runtime_error {
 public:
  PreviewAbortedError() : std::runtime_error("preview aborted") {}
};

}  // namespace gitcg
