// V8-compatible stable ordering, including comparators with mixed NaN keys.
#pragma once
#include <functional>
#include <vector>
namespace gitcg::query {
void sortLikeJavaScript(std::vector<int>& values, const std::function<bool(int, int)>& less);
}  // namespace gitcg::query
