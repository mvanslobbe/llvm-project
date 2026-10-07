//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03

// <string>

// basic_string<charT,traits,Allocator>&
//   operator=(basic_string<charT,traits,Allocator>&& str); // constexpr since C++20

#include <string>
#include <cassert>

#include "test_macros.h"
#include "test_allocator.h"
#include "min_allocator.h"
#include "asan_testing.h"
#include "allocators.h"

template <class S>
TEST_CONSTEXPR_CXX20 void test(S s1, S s2) {
  S s0 = s2;
  s1   = std::move(s2);
  LIBCPP_ASSERT(s1.__invariants());
  LIBCPP_ASSERT(s2.__invariants());
  assert(s1 == s0);
  assert(s1.capacity() >= s1.size());
  LIBCPP_ASSERT(is_string_asan_correct(s0));
  LIBCPP_ASSERT(is_string_asan_correct(s1));
  LIBCPP_ASSERT(is_string_asan_correct(s2));
}

template <class S>
TEST_CONSTEXPR_CXX20 void test_string() {
  test(S(), S());
  test(S("1"), S());
  test(S(), S("1"));
  test(S("1"), S("2"));
  test(S("1"), S("2"));

  test(S(), S("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
  test(S("123456789"), S("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
  test(S("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"), S("123456789"));
  test(S("1234567890123456789012345678901234567890123456789012345678901234567890"),
       S("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
  test(S("1234567890123456789012345678901234567890123456789012345678901234567890"
         "1234567890123456789012345678901234567890123456789012345678901234567890"),
       S("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
}

// The allocator propagates on copy assignment but not on move assignment. Move assigning from a string with an
// unequal allocator must copy the characters and keep the destination's allocator.
TEST_CONSTEXPR_CXX20 void test_unequal_non_pocma_allocator() {
  using A              = POCCAAllocator<char>;
  using S              = std::basic_string<char, std::char_traits<char>, A>;
  const char* values[] = {"", "1", "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"};
  for (const char* v1 : values) {
    for (const char* v2 : values) {
      bool copy_assigned_into = false;
      S s1(v1, A(1, &copy_assigned_into));
      S s2(v2, A(2, nullptr));
      s1 = std::move(s2);
      LIBCPP_ASSERT(s1.__invariants());
      LIBCPP_ASSERT(s2.__invariants());
      assert(s1 == v2);
      assert(s1.get_allocator() == A(1, nullptr));
      assert(s2.get_allocator() == A(2, nullptr));
      assert(!copy_assigned_into);
      LIBCPP_ASSERT(is_string_asan_correct(s1));
      LIBCPP_ASSERT(is_string_asan_correct(s2));
    }
  }
}

TEST_CONSTEXPR_CXX20 bool test() {
  test_string<std::string>();
#if TEST_STD_VER >= 11
  test_string<std::basic_string<char, std::char_traits<char>, min_allocator<char>>>();
  test_string<std::basic_string<char, std::char_traits<char>, safe_allocator<char>>>();
  test_unequal_non_pocma_allocator();
#endif

  return true;
}

int main(int, char**) {
  test();
#if TEST_STD_VER > 17
  static_assert(test());
#endif

  return 0;
}
