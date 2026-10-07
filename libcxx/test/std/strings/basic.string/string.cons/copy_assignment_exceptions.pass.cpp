//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, no-exceptions

// <string>

// basic_string& operator=(const basic_string& str);
// basic_string& assign(const basic_string& str);

// [string.require]/2: if a member function throws, it has no other effect on the string. Check that a failed
// allocation while copy assigning a longer string leaves the destination unchanged.

#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <string>

#include "test_macros.h"

static int allocations_left = -1; // allocate() throws when this reaches 0; -1 never throws.

template <class T>
struct ThrowingAllocator {
  typedef T value_type;

  ThrowingAllocator() {}
  template <class U>
  ThrowingAllocator(const ThrowingAllocator<U>&) {}

  T* allocate(std::size_t n) {
    if (allocations_left == 0)
      throw std::bad_alloc();
    if (allocations_left > 0)
      --allocations_left;
    return std::allocator<T>().allocate(n);
  }
  void deallocate(T* p, std::size_t n) { std::allocator<T>().deallocate(p, n); }

  template <class U>
  friend bool operator==(const ThrowingAllocator&, const ThrowingAllocator<U>&) {
    return true;
  }
  template <class U>
  friend bool operator!=(const ThrowingAllocator&, const ThrowingAllocator<U>&) {
    return false;
  }
};

typedef std::basic_string<char, std::char_traits<char>, ThrowingAllocator<char> > S;

template <class Assign>
void test(Assign assign) {
  const std::size_t sizes[] = {0, 5, 22, 23, 40, 47, 48, 100};
  for (std::size_t i = 0; i != sizeof(sizes) / sizeof(sizes[0]); ++i) {
    for (std::size_t j = 0; j != sizeof(sizes) / sizeof(sizes[0]); ++j) {
      S dst(sizes[i], 'd');
      const S src(sizes[j], 's');
      const S before   = dst;
      allocations_left = 0;
      try {
        assign(dst, src);
        assert(dst == src);
      } catch (const std::bad_alloc&) {
        assert(dst == before);
      }
      allocations_left = -1;
      LIBCPP_ASSERT(dst.__invariants());
    }
  }
}

int main(int, char**) {
  test([](S& dst, const S& src) { dst = src; });
  test([](S& dst, const S& src) { dst.assign(src); });

  return 0;
}
