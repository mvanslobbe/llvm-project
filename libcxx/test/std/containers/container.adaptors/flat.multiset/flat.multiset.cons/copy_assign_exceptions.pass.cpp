//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: no-exceptions

// <flat_set>

// flat_multiset& operator=(const flat_multiset&);

// Test that the invariants are restored if copying the underlying containers throws.

#include <algorithm>
#include <cassert>
#include <deque>
#include <flat_set>
#include <functional>
#include <vector>

#include "test_macros.h"

struct ThrowingCopy {
  static inline int copies_until_throw = -1; // -1: never throw

  int value;

  ThrowingCopy(int v) : value(v) {}
  ThrowingCopy(const ThrowingCopy& other) : value(other.value) { maybe_throw(); }
  ThrowingCopy& operator=(const ThrowingCopy& other) {
    maybe_throw();
    value = other.value;
    return *this;
  }

  static void maybe_throw() {
    if (copies_until_throw >= 0 && copies_until_throw-- == 0)
      throw 42;
  }

  friend bool operator==(const ThrowingCopy&, const ThrowingCopy&)  = default;
  friend auto operator<=>(const ThrowingCopy&, const ThrowingCopy&) = default;
};

template <class M>
void check_invariant(const M& m) {
  assert(std::adjacent_find(m.begin(), m.end(), [&](const auto& x, const auto& y) { return m.key_comp()(y, x); }) ==
         m.end());
}

template <class KeyContainer>
void test() {
  using Key = typename KeyContainer::value_type;
  using M   = std::flat_multiset<Key, std::less<Key>, KeyContainer>;

  // The destination is shorter, equal, or longer, so the containers both reallocate and
  // assign element by element.
  for (int dst_size : {1, 3, 5}) {
    for (int n = 0; n < 12; ++n) {
      M src;
      for (int i = 0; i < 3; ++i)
        src.emplace(10 + i);
      M dst;
      for (int i = 0; i < dst_size; ++i)
        dst.emplace(i);

      ThrowingCopy::copies_until_throw = n;
      try {
        dst = src;
      } catch (int) {
      }
      ThrowingCopy::copies_until_throw = -1;
      check_invariant(dst);
      check_invariant(src);
    }
  }
}

int main(int, char**) {
  test<std::vector<ThrowingCopy>>();
  test<std::deque<ThrowingCopy>>();

  return 0;
}
