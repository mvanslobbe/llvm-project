//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: no-exceptions

// <unordered_map>

// Check that the unordered_map copy constructor does not leak nodes when a node allocation
// throws partway through copying the elements. The already-copied nodes must be released even
// though the container itself never finishes constructing.
//
// Also check the copy-assignment-into-empty path, which shares the same node-copying helper.
// There the assigned-to container stays alive, so after a throw it must be left in a valid
// (usable, leak-free) state with no dangling bucket pointers.
//
// Finally check copy assignment into a non-empty map, which reuses the destination's nodes and
// assigns the source's values into them. If such an assignment throws, the destination must
// still be consistent: every element it contains can be found, and no key appears twice.

#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <new>
#include <unordered_map>
#include <utility>

#include "count_new.h"
#include "test_macros.h"

// An allocator that throws std::bad_alloc once a shared countdown reaches zero. It forwards
// to std::allocator (operator new) so that count_new.h can observe leaked nodes.
template <class T>
struct CountedThrowingAllocator {
  typedef T value_type;

  int* countdown_;

  explicit CountedThrowingAllocator(int& countdown) : countdown_(&countdown) {}

  template <class U>
  CountedThrowingAllocator(const CountedThrowingAllocator<U>& other) : countdown_(other.countdown_) {}

  T* allocate(std::size_t n) {
    if (*countdown_ == 0)
      throw std::bad_alloc();
    --*countdown_;
    return std::allocator<T>().allocate(n);
  }

  void deallocate(T* p, std::size_t n) { std::allocator<T>().deallocate(p, n); }

  template <class U>
  friend bool operator==(const CountedThrowingAllocator&, const CountedThrowingAllocator<U>&) {
    return true;
  }

  template <class U>
  friend bool operator!=(const CountedThrowingAllocator&, const CountedThrowingAllocator<U>&) {
    return false;
  }
};

// A value type whose copy assignment throws once a shared countdown reaches zero.
struct ThrowingAssign {
  static int countdown;
  int value;

  ThrowingAssign(int v) : value(v) {}
  ThrowingAssign(const ThrowingAssign&) = default;
  ThrowingAssign& operator=(const ThrowingAssign& other) {
    if (countdown == 0)
      throw 42;
    if (countdown > 0)
      --countdown;
    value = other.value;
    return *this;
  }
};

int ThrowingAssign::countdown = -1;

void test_copy_assign_reusing_nodes() {
  typedef std::unordered_map<int, ThrowingAssign> Map;

  Map src;
  for (int i = 0; i < 16; ++i)
    src.insert(std::make_pair(i, ThrowingAssign(i)));

  for (int dst_size = 1; dst_size <= 24; dst_size += 7) {
    // Only the reused nodes are assigned into, at most min(dst_size, src.size()) of them.
    for (int fail_at = 0; fail_at < dst_size && fail_at < 16; ++fail_at) {
      const int outstanding_before = globalMemCounter.outstanding_new;
      {
        Map dst;
        for (int i = 0; i < dst_size; ++i)
          dst.insert(std::make_pair(100 + i, ThrowingAssign(i)));

        ThrowingAssign::countdown = fail_at;
        try {
          dst = src;
          assert(false);
        } catch (int) {
        }
        ThrowingAssign::countdown = -1;

        // Every element that can be reached by iteration must also be found by lookup.
        std::size_t n = 0;
        for (Map::iterator it = dst.begin(); it != dst.end(); ++it, ++n)
          assert(dst.find(it->first) == it);
        assert(n == dst.size());

        // Inserting keys again must not create duplicates.
        for (int i = 0; i < 16; ++i)
          dst.insert(std::make_pair(i, ThrowingAssign(i)));
        for (int i = 0; i < 16; ++i)
          assert(dst.count(i) == 1);
      }
      assert(globalMemCounter.outstanding_new == outstanding_before);
    }
  }
}

int main(int, char**) {
  test_copy_assign_reusing_nodes();

  typedef CountedThrowingAllocator<std::pair<const int, int> > Alloc;
  typedef std::unordered_map<int, int, std::hash<int>, std::equal_to<int>, Alloc> Map;

  const int never_throw = 1 << 30;
  int countdown         = never_throw;

  // Build a source map large enough that copying it performs several node allocations. The
  // allocator is propagated by select_on_container_copy_construction, so the copy shares this
  // same countdown.
  Map src((Alloc(countdown)));
  for (int i = 0; i < 16; ++i)
    src.insert(std::make_pair(i, i));

  // Make a node allocation fail at every position partway through the copy and verify that no
  // nodes are leaked. `outstanding_before` captures the allocations held by the source so the
  // check is unaffected by it.
  for (int fail_at = 0; fail_at < 32; ++fail_at) {
    const int outstanding_before = globalMemCounter.outstanding_new;

    countdown = fail_at;
    try {
      Map copy(src);
      (void)copy;
    } catch (const std::bad_alloc&) {
    }
    countdown = never_throw;

    assert(globalMemCounter.outstanding_new == outstanding_before);
  }

  // Same, but through copy-assignment into an empty (default-constructed) map. The destination
  // survives the exception, so it must be left valid: we exercise it with insert()/find() (which
  // read the bucket array and would dereference a stale entry under a sanitizer) and confirm no
  // nodes are leaked once it goes out of scope.
  for (int fail_at = 0; fail_at < 32; ++fail_at) {
    const int outstanding_before = globalMemCounter.outstanding_new;
    {
      Map dst((Alloc(countdown)));
      countdown = fail_at;
      try {
        dst = src;
      } catch (const std::bad_alloc&) {
      }
      countdown = never_throw;

      // The container must be in a valid state whether or not the assignment threw.
      for (int i = 0; i < 16; ++i)
        dst.insert(std::make_pair(i, i));
      for (int i = 0; i < 16; ++i)
        assert(dst.find(i) != dst.end());
    }
    assert(globalMemCounter.outstanding_new == outstanding_before);
  }

  return 0;
}
