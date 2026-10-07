//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: no-exceptions

// <memory>

// Make sure that uninitialized_copy, uninitialized_copy_n and their ranges:: counterparts destroy every object
// they have constructed when incrementing the input iterator throws ([specialized.algorithms.general]/2).

#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>

#include "../buffer.h"
#include "../counted.h"
#include "test_macros.h"

// A forward iterator whose operator++ throws when the countdown reaches zero.
struct ThrowingIncrementIterator {
  using iterator_category = std::forward_iterator_tag;
  using difference_type   = std::ptrdiff_t;
  using value_type        = Counted;
  using pointer           = const Counted*;
  using reference         = const Counted&;

  static int throw_after; // Number of increments that succeed; -1 never throws.

  const Counted* ptr = nullptr;

  ThrowingIncrementIterator() = default;
  explicit ThrowingIncrementIterator(const Counted* p) : ptr(p) {}

  reference operator*() const { return *ptr; }

  ThrowingIncrementIterator& operator++() {
    if (throw_after == 0)
      throw 42;
    if (throw_after > 0)
      --throw_after;
    ++ptr;
    return *this;
  }
  ThrowingIncrementIterator operator++(int) {
    ThrowingIncrementIterator tmp = *this;
    ++*this;
    return tmp;
  }

  friend bool operator==(const ThrowingIncrementIterator& x, const ThrowingIncrementIterator& y) {
    return x.ptr == y.ptr;
  }
  friend bool operator!=(const ThrowingIncrementIterator& x, const ThrowingIncrementIterator& y) {
    return x.ptr != y.ptr;
  }
};

int ThrowingIncrementIterator::throw_after = -1;

template <class F>
void test_throw_after(F copy) {
  constexpr int N = 5;
  for (int k = 0; k < N - 1; ++k) {
    Counted in[N] = {Counted(1), Counted(2), Counted(3), Counted(4), Counted(5)};
    Buffer<Counted, N> out;
    Counted::reset();
    ThrowingIncrementIterator::throw_after = k; // Throw after k + 1 objects have been constructed.
    try {
      copy(ThrowingIncrementIterator(in), ThrowingIncrementIterator(in + N), N, out.begin(), out.end());
      assert(false);
    } catch (int) {
    }
    assert(Counted::total_objects == k + 1);
    assert(Counted::current_objects == 0);
    ThrowingIncrementIterator::throw_after = -1;
  }
}

int main(int, char**) {
  using It = ThrowingIncrementIterator;

  test_throw_after([](It first, It last, int, Counted* out, Counted*) { std::uninitialized_copy(first, last, out); });
  test_throw_after([](It first, It, int n, Counted* out, Counted*) { std::uninitialized_copy_n(first, n, out); });
#if TEST_STD_VER >= 20
  test_throw_after([](It first, It last, int, Counted* out, Counted* out_last) {
    std::ranges::uninitialized_copy(first, last, out, out_last);
  });
  test_throw_after([](It first, It, int n, Counted* out, Counted* out_last) {
    std::ranges::uninitialized_copy_n(first, n, out, out_last);
  });
#endif

  return 0;
}
