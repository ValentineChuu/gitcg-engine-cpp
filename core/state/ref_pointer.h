// Reference-counted pointer to an immutable state node. Not thread-safe on purpose: one game runs on one thread,
// and non-atomic counts keep copying a snapshot (the core keeps many) cheap.
#pragma once

#include <cstdint>
#include <utility>

namespace gitcg {

class RefCounted {
 public:
  RefCounted() = default;
  RefCounted(const RefCounted&) : referenceCount_(0) {}
  RefCounted& operator=(const RefCounted&) { return *this; }

  void addReference() const { ++referenceCount_; }
  bool releaseReference() const { return --referenceCount_ == 0; }
  bool isUnique() const { return referenceCount_ == 1; }

 private:
  mutable uint32_t referenceCount_ = 0;
};

template <typename T>
class RefPointer {
 public:
  RefPointer() = default;
  RefPointer(std::nullptr_t) {}
  explicit RefPointer(T* pointer) : pointer_(pointer) { retain(); }
  RefPointer(const RefPointer& other) : pointer_(other.pointer_) { retain(); }
  RefPointer(RefPointer&& other) noexcept : pointer_(std::exchange(other.pointer_, nullptr)) {}
  template <typename U>
  RefPointer(const RefPointer<U>& other) : pointer_(other.get()) { retain(); }
  ~RefPointer() { release(); }

  RefPointer& operator=(const RefPointer& other) {
    if (pointer_ != other.pointer_) {
      other.retain();
      release();
      pointer_ = other.pointer_;
    }
    return *this;
  }
  RefPointer& operator=(RefPointer&& other) noexcept {
    if (this != &other) {
      release();
      pointer_ = std::exchange(other.pointer_, nullptr);
    }
    return *this;
  }

  T* get() const { return pointer_; }
  T& operator*() const { return *pointer_; }
  T* operator->() const { return pointer_; }
  explicit operator bool() const { return pointer_ != nullptr; }
  bool operator==(const RefPointer& other) const { return pointer_ == other.pointer_; }
  bool operator!=(const RefPointer& other) const { return pointer_ != other.pointer_; }

 private:
  void retain() const {
    if (pointer_) pointer_->addReference();
  }
  void release() {
    if (pointer_ && pointer_->releaseReference()) delete pointer_;
    pointer_ = nullptr;
  }

  T* pointer_ = nullptr;
};

template <typename T, typename... Arguments>
RefPointer<T> makeRef(Arguments&&... arguments) {
  return RefPointer<T>(new T(std::forward<Arguments>(arguments)...));
}

// Copy-on-write: a node that only this pointer holds is edited in place, any other is copied first.
template <typename T>
T& editable(RefPointer<T>& pointer) {
  if (!pointer->isUnique()) pointer = makeRef<T>(*pointer);
  return *pointer;
}

}  // namespace gitcg
