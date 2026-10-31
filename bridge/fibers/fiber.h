// A cooperative fiber (ucontext) with its own stack. It runs until it yields or finishes; exceptions inside it are
// caught at its top and kept for whoever waits on it.
#pragma once

#include <exception>
#include <functional>
#include <ucontext.h>

namespace gitcg {

class Fiber {
 public:
  explicit Fiber(std::function<void()> body, size_t stackSize = 1 << 20);
  ~Fiber();
  Fiber(const Fiber&) = delete;
  Fiber& operator=(const Fiber&) = delete;

  // Runs until the fiber yields or finishes; returns to the caller then.
  void resume();
  // Called from inside the fiber: back to whoever resumed it.
  void yield();

  bool finished() const { return finished_; }
  std::exception_ptr error() const { return error_; }

 private:
  static void entry(unsigned int high, unsigned int low);

  std::function<void()> body_;
  void* stack_ = nullptr;
  size_t stackSize_;
  ucontext_t context_{};
  ucontext_t caller_{};
  bool started_ = false;
  bool finished_ = false;
  std::exception_ptr error_;
};

}  // namespace gitcg
