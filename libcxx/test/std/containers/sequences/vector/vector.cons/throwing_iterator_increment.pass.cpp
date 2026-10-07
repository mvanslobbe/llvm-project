//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: no-exceptions

// <vector>

// Check that every element constructed from a forward iterator range is destroyed when incrementing the iterator
// throws, in vector(first, last), vector(first, last, alloc), assign(first, last) and insert(pos, first, last).

#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <vector>

#include "test_macros.h"

struct Elem {
  static int live;
  int value;
  Elem(int v) : value(v) { ++live; }
  Elem(const Elem& other) : value(other.value) { ++live; }
  Elem& operator=(const Elem&) = default;
  ~Elem() { --live; }
};

int Elem::live = 0;

// A forward iterator whose operator++ throws when the countdown reaches zero.
struct ThrowingIncrementIterator {
  using iterator_category = std::forward_iterator_tag;
  using difference_type   = std::ptrdiff_t;
  using value_type        = Elem;
  using pointer           = const Elem*;
  using reference         = const Elem&;

  static int throw_after; // Number of increments that succeed; -1 never throws.

  const Elem* ptr;

  explicit ThrowingIncrementIterator(const Elem* p = nullptr) : ptr(p) {}

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
void test(F f) {
  const int N      = 4;
  const Elem in[N] = {Elem(1), Elem(2), Elem(3), Elem(4)};
  // The implementation may walk the range once to compute its length before copying, so let the throw happen at
  // every possible increment, during either pass.
  for (int k = 0; k < 2 * N; ++k) {
    {
      std::vector<Elem> v;
      v.reserve(1); // Not enough room, so that insert and assign construct into new storage.
      v.push_back(Elem(0));
      ThrowingIncrementIterator::throw_after = k;
      try {
        f(v, ThrowingIncrementIterator(in), ThrowingIncrementIterator(in + N));
      } catch (int) {
      }
      ThrowingIncrementIterator::throw_after = -1;
      assert(Elem::live == N + static_cast<int>(v.size()));
    }
    assert(Elem::live == N);
  }
}

int main(int, char**) {
  using It = ThrowingIncrementIterator;

  test([](std::vector<Elem>&, It first, It last) {
    std::vector<Elem> w(first, last);
    (void)w;
  });
  test([](std::vector<Elem>&, It first, It last) {
    std::vector<Elem> w(first, last, std::allocator<Elem>());
    (void)w;
  });
  test([](std::vector<Elem>& v, It first, It last) { v.assign(first, last); });
  test([](std::vector<Elem>& v, It first, It last) { v.insert(v.begin(), first, last); });
  test([](std::vector<Elem>& v, It first, It last) { v.insert(v.end(), first, last); });

  return 0;
}
