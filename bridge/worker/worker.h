// The stdio worker (reference/bridge/worker.mjs): create, copy, replies, respond, stop and shutdown; one JSON
// message per line. Test snapshots are accepted only with an explicit constructor option.
#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "bridge/fibers/scheduler.h"
#include "bridge/session/game_session.h"
#include "core/definitions/game_data.h"

namespace gitcg {

class Worker {
 public:
  explicit Worker(const GameData& data, bool allowTestStates = false) : data_(data), allowTestStates_(allowTestStates) {}

  // Handles one input line; false after "shutdown".
  bool handleLine(const std::string& line);

 private:
  void create(const JsonValue& message);
  void copy(const JsonValue& message);
  void emit(const JsonValue& message);
  void settle();  // run fibers, then emit waiting decisions in the order they were asked

  const GameData& data_;
  bool allowTestStates_;
  Scheduler scheduler_;
  std::map<int, std::unique_ptr<GameSession>> sessions_;
  std::vector<std::pair<int, int>> queuedDecisions_;  // (game, who)
};

}  // namespace gitcg
