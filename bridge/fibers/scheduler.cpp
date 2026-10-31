#include "bridge/fibers/scheduler.h"

#include <algorithm>
#include <stdexcept>

namespace gitcg {

Scheduler::Task* Scheduler::spawn(std::function<void()> body, std::function<void(Task&)> onFinish) {
  auto task = std::make_unique<Task>();
  task->fiber = std::make_unique<Fiber>(std::move(body));
  task->onFinish = std::move(onFinish);
  Task* pointer = task.get();
  tasks_.push_back(std::move(task));
  wake(pointer);
  return pointer;
}

void Scheduler::wait() {
  Task* task = current_;
  if (!task) throw std::logic_error("wait() outside a fiber");
  task->fiber->yield();
  if (task->cancelled) throw FiberCancelled();
}

void Scheduler::wake(Task* task) {
  if (task->queued || task->fiber->finished()) return;
  task->queued = true;
  runnable_.push_back(task);
}

void Scheduler::cancel(Task* task) {
  if (task->fiber->finished()) return;
  task->cancelled = true;
  wake(task);
}

void Scheduler::runUntilIdle() {
  while (!runnable_.empty()) {
    Task* task = runnable_.front();
    runnable_.pop_front();
    task->queued = false;
    Task* previous = current_;
    current_ = task;
    task->fiber->resume();
    current_ = previous;
    if (task->fiber->finished()) {
      if (task->onFinish) task->onFinish(*task);
      std::erase_if(tasks_, [&](const std::unique_ptr<Task>& owned) { return owned.get() == task; });
    }
  }
}

}  // namespace gitcg
