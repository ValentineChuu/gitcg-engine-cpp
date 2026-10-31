// Runs fibers cooperatively on the worker's main thread: a fiber waits until something wakes it; the scheduler
// runs woken fibers in the order they were woken, until none can run.
#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <vector>

#include "bridge/fibers/fiber.h"

namespace gitcg {

// Thrown inside a waiting fiber that is cancelled, to unwind it.
class FiberCancelled : public std::exception {};

class Scheduler {
 public:
  struct Task {
    std::unique_ptr<Fiber> fiber;
    std::function<void(Task&)> onFinish;
    bool cancelled = false;
    bool queued = false;
  };

  // Creates a fiber and queues it to run.
  Task* spawn(std::function<void()> body, std::function<void(Task&)> onFinish = nullptr);
  // From inside a fiber: suspend until woken. Throws FiberCancelled when cancelled.
  void wait();
  void wake(Task* task);
  // Wakes a waiting fiber so that it unwinds.
  void cancel(Task* task);
  void runUntilIdle();
  Task* current() const { return current_; }

 private:
  std::vector<std::unique_ptr<Task>> tasks_;
  std::deque<Task*> runnable_;
  Task* current_ = nullptr;
};

}  // namespace gitcg
