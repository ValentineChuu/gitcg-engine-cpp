// TimSort comparison schedule used by Node 26.11.1 / V8 14.6 (AGPL-3.0).
// Algorithm reference: nodejs/node v26.11.1, deps/v8/third_party/v8/builtins/array-sort.tq.
// Mixed NaN keys make Piovium's comparator non-transitive, so std::stable_sort is insufficient.
#include "query/ordering.h"

#include <algorithm>

namespace gitcg::query {
namespace {
class Sorter {
 public:
  Sorter(std::vector<int>& values, const std::function<bool(int, int)>& less) : values_(values), less_(less) {}
  void run() {
    int size = values_.size();
    if (size < 2) return;
    int minimum = size, remainder = 0;
    while (minimum >= 64) { remainder |= minimum & 1; minimum >>= 1; }
    minimum += remainder;
    int low = 0;
    while (low < size) {
      int length = naturalRun(low, size);
      if (length < minimum) {
        int forced = std::min(minimum, size - low);
        insertion(low, low + length, low + forced);
        length = forced;
      }
      pending_.push_back({low, length});
      collapse(false);
      low += length;
    }
    collapse(true);
  }
 private:
  static void copy(const std::vector<int>& from, int source, std::vector<int>& to, int target, int count) {
    if (&from == &to && source < target) {
      for (int i = count - 1; i >= 0; --i) to[target + i] = from[source + i];
    } else for (int i = 0; i < count; ++i) to[target + i] = from[source + i];
  }
  int naturalRun(int low, int high) {
    if (low + 1 == high) return 1;
    bool descending = less_(values_[low + 1], values_[low]);
    int end = low + 2;
    while (end < high && less_(values_[end], values_[end - 1]) == descending) ++end;
    if (descending) std::reverse(values_.begin() + low, values_.begin() + end);
    return end - low;
  }
  void insertion(int low, int start, int high) {
    for (; start < high; ++start) {
      int pivot = values_[start], left = low, right = start;
      while (left < right) {
        int middle = left + (right - left) / 2;
        if (less_(pivot, values_[middle])) right = middle;
        else left = middle + 1;
      }
      copy(values_, left, values_, left + 1, start - left);
      values_[left] = pivot;
    }
  }
  int gallop(const std::vector<int>& values, int key, int base, int length, int hint, bool upper) {
    auto before = [&](int index) { return upper ? !less_(key, values[base + index]) : less_(values[base + index], key); };
    int last = 0, offset = 1;
    if (before(hint)) {
      int maximum = length - hint;
      while (offset < maximum && before(hint + offset)) { last = offset; offset = offset * 2 + 1; }
      offset = std::min(offset, maximum);
      last += hint; offset += hint;
    } else {
      int maximum = hint + 1;
      while (offset < maximum && !before(hint - offset)) { last = offset; offset = offset * 2 + 1; }
      offset = std::min(offset, maximum);
      int previous = last;
      last = hint - offset; offset = hint - previous;
    }
    ++last;
    while (last < offset) {
      int middle = last + (offset - last) / 2;
      if (before(middle)) last = middle + 1;
      else offset = middle;
    }
    return offset;
  }
  void mergeLow(int baseA, int lengthA, int baseB, int lengthB) {
    std::vector<int> temp(values_.begin() + baseA, values_.begin() + baseA + lengthA);
    int destination = baseA, cursorA = 0, cursorB = baseB;
    auto finish = [&] { if (lengthA) copy(temp, cursorA, values_, destination, lengthA); };
    auto lastA = [&] {
      copy(values_, cursorB, values_, destination, lengthB);
      values_[destination + lengthB] = temp[cursorA];
    };
    values_[destination++] = values_[cursorB++];
    if (--lengthB == 0) { finish(); return; }
    if (lengthA == 1) { lastA(); return; }
    int minimum = minimumGallop_;
    while (true) {
      int winsA = 0, winsB = 0;
      while (true) {
        if (less_(values_[cursorB], temp[cursorA])) {
          values_[destination++] = values_[cursorB++];
          ++winsB; winsA = 0;
          if (--lengthB == 0) { finish(); return; }
          if (winsB >= minimum) break;
        } else {
          values_[destination++] = temp[cursorA++];
          ++winsA; winsB = 0;
          if (--lengthA == 1) { lastA(); return; }
          if (winsA >= minimum) break;
        }
      }
      ++minimum;
      bool first = true;
      while (first || winsA >= 7 || winsB >= 7) {
        first = false;
        minimum = std::max(1, minimum - 1); minimumGallop_ = minimum;
        winsA = gallop(temp, values_[cursorB], cursorA, lengthA, 0, true);
        if (winsA) {
          copy(temp, cursorA, values_, destination, winsA);
          destination += winsA; cursorA += winsA; lengthA -= winsA;
          if (lengthA == 1) { lastA(); return; }
          if (lengthA == 0) return;
        }
        values_[destination++] = values_[cursorB++];
        if (--lengthB == 0) { finish(); return; }
        winsB = gallop(values_, temp[cursorA], cursorB, lengthB, 0, false);
        if (winsB) {
          copy(values_, cursorB, values_, destination, winsB);
          destination += winsB; cursorB += winsB; lengthB -= winsB;
          if (lengthB == 0) { finish(); return; }
        }
        values_[destination++] = temp[cursorA++];
        if (--lengthA == 1) { lastA(); return; }
      }
      minimumGallop_ = ++minimum;
    }
  }
  void mergeHigh(int baseA, int lengthA, int baseB, int lengthB) {
    std::vector<int> temp(values_.begin() + baseB, values_.begin() + baseB + lengthB);
    int destination = baseB + lengthB - 1, cursorA = baseA + lengthA - 1, cursorB = lengthB - 1;
    auto finish = [&] { if (lengthB) copy(temp, 0, values_, destination - lengthB + 1, lengthB); };
    auto lastB = [&] {
      destination -= lengthA; cursorA -= lengthA;
      copy(values_, cursorA + 1, values_, destination + 1, lengthA);
      values_[destination] = temp[cursorB];
    };
    values_[destination--] = values_[cursorA--];
    if (--lengthA == 0) { finish(); return; }
    if (lengthB == 1) { lastB(); return; }
    int minimum = minimumGallop_;
    while (true) {
      int winsA = 0, winsB = 0;
      while (true) {
        if (less_(temp[cursorB], values_[cursorA])) {
          values_[destination--] = values_[cursorA--];
          ++winsA; winsB = 0;
          if (--lengthA == 0) { finish(); return; }
          if (winsA >= minimum) break;
        } else {
          values_[destination--] = temp[cursorB--];
          ++winsB; winsA = 0;
          if (--lengthB == 1) { lastB(); return; }
          if (winsB >= minimum) break;
        }
      }
      ++minimum;
      bool first = true;
      while (first || winsA >= 7 || winsB >= 7) {
        first = false;
        minimum = std::max(1, minimum - 1); minimumGallop_ = minimum;
        winsA = lengthA - gallop(values_, temp[cursorB], baseA, lengthA, lengthA - 1, true);
        if (winsA) {
          destination -= winsA; cursorA -= winsA;
          copy(values_, cursorA + 1, values_, destination + 1, winsA);
          lengthA -= winsA;
          if (lengthA == 0) { finish(); return; }
        }
        values_[destination--] = temp[cursorB--];
        if (--lengthB == 1) { lastB(); return; }
        winsB = lengthB - gallop(temp, values_[cursorA], 0, lengthB, lengthB - 1, false);
        if (winsB) {
          destination -= winsB; cursorB -= winsB;
          copy(temp, cursorB + 1, values_, destination + 1, winsB);
          lengthB -= winsB;
          if (lengthB == 1) { lastB(); return; }
          if (lengthB == 0) return;
        }
        values_[destination--] = values_[cursorA--];
        if (--lengthA == 0) { finish(); return; }
      }
      minimumGallop_ = ++minimum;
    }
  }
  void merge(int index) {
    auto [baseA, lengthA] = pending_[index];
    auto [baseB, lengthB] = pending_[index + 1];
    pending_[index].second += lengthB;
    pending_.erase(pending_.begin() + index + 1);
    int skip = gallop(values_, values_[baseB], baseA, lengthA, 0, true);
    baseA += skip; lengthA -= skip;
    if (!lengthA) return;
    lengthB = gallop(values_, values_[baseA + lengthA - 1], baseB, lengthB, lengthB - 1, false);
    if (!lengthB) return;
    if (lengthA <= lengthB) mergeLow(baseA, lengthA, baseB, lengthB);
    else mergeHigh(baseA, lengthA, baseB, lengthB);
  }
  bool invariant(int index) const {
    return index < 2 || pending_[index - 2].second > pending_[index - 1].second + pending_[index].second;
  }
  void collapse(bool force) {
    while (pending_.size() > 1) {
      int index = pending_.size() - 2;
      if (force || !invariant(index + 1) || !invariant(index)) {
        if (index > 0 && pending_[index - 1].second < pending_[index + 1].second) --index;
      } else if (pending_[index].second > pending_[index + 1].second) break;
      merge(index);
    }
  }
  std::vector<int>& values_;
  const std::function<bool(int, int)>& less_;
  std::vector<std::pair<int, int>> pending_;
  int minimumGallop_ = 7;
};
}  // namespace
void sortLikeJavaScript(std::vector<int>& values, const std::function<bool(int, int)>& less) { Sorter(values, less).run(); }
}  // namespace gitcg::query
