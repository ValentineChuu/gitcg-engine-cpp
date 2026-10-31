#include "bridge/fibers/fiber.h"

#include <cstdint>
#include <stdexcept>
#include <sys/mman.h>

namespace gitcg {

Fiber::Fiber(std::function<void()> body, size_t stackSize) : body_(std::move(body)), stackSize_(stackSize) {
  // Untouched stack pages cost nothing; the lowest page guards against overflow.
  stack_ = mmap(nullptr, stackSize_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (stack_ == MAP_FAILED) throw std::runtime_error("fiber stack allocation failed");
  mprotect(stack_, 4096, PROT_NONE);
  getcontext(&context_);
  context_.uc_stack.ss_sp = stack_;
  context_.uc_stack.ss_size = stackSize_;
  context_.uc_link = &caller_;
  auto address = reinterpret_cast<uintptr_t>(this);
  makecontext(&context_, reinterpret_cast<void (*)()>(&Fiber::entry), 2, static_cast<unsigned int>(address >> 32),
              static_cast<unsigned int>(address & 0xffffffffu));
}

Fiber::~Fiber() {
  munmap(stack_, stackSize_);
}

void Fiber::entry(unsigned int high, unsigned int low) {
  auto* fiber = reinterpret_cast<Fiber*>((static_cast<uintptr_t>(high) << 32) | low);
  try {
    fiber->body_();
  } catch (...) {
    fiber->error_ = std::current_exception();
  }
  fiber->finished_ = true;
  // uc_link returns to the last resumer.
}

void Fiber::resume() {
  if (finished_) return;
  started_ = true;
  swapcontext(&caller_, &context_);
}

void Fiber::yield() {
  swapcontext(&context_, &caller_);
}

}  // namespace gitcg
