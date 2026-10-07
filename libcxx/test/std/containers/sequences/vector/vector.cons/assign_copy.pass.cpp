//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <vector>

// vector& operator=(const vector& c);

#include <vector>
#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include "test_macros.h"
#include "test_allocator.h"
#include "min_allocator.h"
#include "allocators.h"

#if TEST_STD_VER >= 11
// A POCCA allocator that checks that only pointers returned by allocate() are passed to deallocate().
template <class T>
struct NonNullDeallocateAllocator {
  using value_type                             = T;
  using propagate_on_container_copy_assignment = std::true_type;

  int id;

  TEST_CONSTEXPR explicit NonNullDeallocateAllocator(int i) : id(i) {}
  template <class U>
  TEST_CONSTEXPR NonNullDeallocateAllocator(const NonNullDeallocateAllocator<U>& other) : id(other.id) {}

  TEST_CONSTEXPR_CXX20 T* allocate(std::size_t n) { return std::allocator<T>().allocate(n); }
  TEST_CONSTEXPR_CXX20 void deallocate(T* p, std::size_t n) {
    assert(p != nullptr);
    std::allocator<T>().deallocate(p, n);
  }

  friend TEST_CONSTEXPR bool operator==(const NonNullDeallocateAllocator& x, const NonNullDeallocateAllocator& y) {
    return x.id == y.id;
  }
  friend TEST_CONSTEXPR bool operator!=(const NonNullDeallocateAllocator& x, const NonNullDeallocateAllocator& y) {
    return x.id != y.id;
  }
};
#endif

TEST_CONSTEXPR_CXX20 bool tests() {
  {
    std::vector<int, test_allocator<int> > l(3, 2, test_allocator<int>(5));
    std::vector<int, test_allocator<int> > l2(l, test_allocator<int>(3));
    l2 = l;
    assert(l2 == l);
    assert(l2.get_allocator() == test_allocator<int>(3));
  }
  {
    std::vector<int, other_allocator<int> > l(3, 2, other_allocator<int>(5));
    std::vector<int, other_allocator<int> > l2(l, other_allocator<int>(3));
    l2 = l;
    assert(l2 == l);
    assert(l2.get_allocator() == other_allocator<int>(5));
  }
#if TEST_STD_VER >= 11
  {
    // Test with Allocator::propagate_on_container_copy_assignment == false_type
    using Alloc             = NonPOCCAAllocator<int>;
    bool copy_assigned_into = false;
    std::vector<int, Alloc> l(3, 2, Alloc(5, nullptr));
    std::vector<int, Alloc> l2(l, Alloc(3, &copy_assigned_into));
    assert(!copy_assigned_into);
    l2 = l;
    assert(!copy_assigned_into);
    assert(l2 == l);
    assert(l2.get_allocator() == Alloc(3, nullptr));
  }
  {
    // Test with Allocator::propagate_on_container_copy_assignment == true_type
    // and equal allocators
    using Alloc             = POCCAAllocator<int>;
    bool copy_assigned_into = false;
    std::vector<int, Alloc> l(3, 2, Alloc(5, nullptr));
    std::vector<int, Alloc> l2(l, Alloc(5, &copy_assigned_into));
    assert(!copy_assigned_into);
    l2 = l;
    assert(copy_assigned_into);
    assert(l2 == l);
    assert(l2.get_allocator() == Alloc(5, nullptr));
  }
  {
    // Test with Allocator::propagate_on_container_copy_assignment == true_type
    // and unequal allocators
    using Alloc             = POCCAAllocator<int>;
    bool copy_assigned_into = false;
    std::vector<int, Alloc> l(3, 2, Alloc(5, nullptr));
    std::vector<int, Alloc> l2(l, Alloc(3, &copy_assigned_into));
    assert(!copy_assigned_into);
    l2 = l;
    assert(copy_assigned_into);
    assert(l2 == l);
    assert(l2.get_allocator() == Alloc(5, nullptr));
  }
  {
    // Test with Allocator::propagate_on_container_copy_assignment == true_type, unequal allocators,
    // and a destination that has never allocated: nothing may be deallocated.
    using Alloc = NonNullDeallocateAllocator<int>;
    std::vector<int, Alloc> l(3, 2, Alloc(5));
    std::vector<int, Alloc> l2(Alloc(3));
    l2 = l;
    assert(l2 == l);
    assert(l2.get_allocator() == Alloc(5));

    std::vector<int, Alloc> l3(Alloc(7));
    std::vector<int, Alloc> l4(Alloc(8));
    l4 = l3;
    assert(l4.empty());
    assert(l4.get_allocator() == Alloc(7));
  }
  {
    std::vector<int, min_allocator<int> > l(3, 2, min_allocator<int>());
    std::vector<int, min_allocator<int> > l2(l, min_allocator<int>());
    l2 = l;
    assert(l2 == l);
    assert(l2.get_allocator() == min_allocator<int>());
  }
  {
    std::vector<int, safe_allocator<int> > l(3, 2, safe_allocator<int>());
    std::vector<int, safe_allocator<int> > l2(l, safe_allocator<int>());
    l2 = l;
    assert(l2 == l);
    assert(l2.get_allocator() == safe_allocator<int>());
  }
#endif

  return true;
}

int main(int, char**) {
  tests();
#if TEST_STD_VER > 17
  static_assert(tests());
#endif
  return 0;
}
