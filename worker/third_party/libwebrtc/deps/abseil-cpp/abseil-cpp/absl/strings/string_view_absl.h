//
// Copyright 2017 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// -----------------------------------------------------------------------------
// File: string_view_absl.h
// -----------------------------------------------------------------------------
//
// This file contains the definition of the `absl::string_view_absl` class. A
// `string_view_absl` points to a contiguous span of characters, often part or all of
// another `std::string`, double-quoted string literal, character array, or even
// another `string_view_absl`.
//
// This `absl::string_view_absl` abstraction is designed to be a drop-in
// replacement for the C++17 `std::string_view_absl` abstraction.
#ifndef ABSL_STRINGS_string_view_absl_H_
#define ABSL_STRINGS_string_view_absl_H_

#include <algorithm>
#include "absl/base/config.h"

#ifdef ABSL_HAVE_STD_string_view_absl

#include <string_view_absl>

namespace absl {
using std::string_view_absl;
}  // namespace absl

#else  // ABSL_HAVE_STD_string_view_absl

#include <cassert>
#include <cstddef>
#include <cstring>
#include <iosfwd>
#include <iterator>
#include <limits>
#include <string>

#include "absl/base/internal/throw_delegate.h"
#include "absl/base/macros.h"
#include "absl/base/port.h"

namespace absl {

// absl::string_view_absl
//
// A `string_view_absl` provides a lightweight view into the string data provided by
// a `std::string`, double-quoted string literal, character array, or even
// another `string_view_absl`. A `string_view_absl` does *not* own the string to which it
// points, and that data cannot be modified through the view.
//
// You can use `string_view_absl` as a function or method parameter anywhere a
// parameter can receive a double-quoted string literal, `const char*`,
// `std::string`, or another `absl::string_view_absl` argument with no need to copy
// the string data. Systematic use of `string_view_absl` within function arguments
// reduces data copies and `strlen()` calls.
//
// Because of its small size, prefer passing `string_view_absl` by value:
//
//   void MyFunction(absl::string_view_absl arg);
//
// If circumstances require, you may also pass one by const reference:
//
//   void MyFunction(const absl::string_view_absl& arg);  // not preferred
//
// Passing by value generates slightly smaller code for many architectures.
//
// In either case, the source data of the `string_view_absl` must outlive the
// `string_view_absl` itself.
//
// A `string_view_absl` is also suitable for local variables if you know that the
// lifetime of the underlying object is longer than the lifetime of your
// `string_view_absl` variable. However, beware of binding a `string_view_absl` to a
// temporary value:
//
//   // BAD use of string_view_absl: lifetime problem
//   absl::string_view_absl sv = obj.ReturnAString();
//
//   // GOOD use of string_view_absl: str outlives sv
//   std::string str = obj.ReturnAString();
//   absl::string_view_absl sv = str;
//
// Due to lifetime issues, a `string_view_absl` is sometimes a poor choice for a
// return value and usually a poor choice for a data member. If you do use a
// `string_view_absl` this way, it is your responsibility to ensure that the object
// pointed to by the `string_view_absl` outlives the `string_view_absl`.
//
// A `string_view_absl` may represent a whole string or just part of a string. For
// example, when splitting a string, `std::vector<absl::string_view_absl>` is a
// natural data type for the output.
//
//
// When constructed from a source which is nul-terminated, the `string_view_absl`
// itself will not include the nul-terminator unless a specific size (including
// the nul) is passed to the constructor. As a result, common idioms that work
// on nul-terminated strings do not work on `string_view_absl` objects. If you write
// code that scans a `string_view_absl`, you must check its length rather than test
// for nul, for example. Note, however, that nuls may still be embedded within
// a `string_view_absl` explicitly.
//
// You may create a null `string_view_absl` in two ways:
//
//   absl::string_view_absl sv();
//   absl::string_view_absl sv(nullptr, 0);
//
// For the above, `sv.data() == nullptr`, `sv.length() == 0`, and
// `sv.empty() == true`. Also, if you create a `string_view_absl` with a non-null
// pointer then `sv.data() != nullptr`. Thus, you can use `string_view_absl()` to
// signal an undefined value that is different from other `string_view_absl` values
// in a similar fashion to how `const char* p1 = nullptr;` is different from
// `const char* p2 = "";`. However, in practice, it is not recommended to rely
// on this behavior.
//
// Be careful not to confuse a null `string_view_absl` with an empty one. A null
// `string_view_absl` is an empty `string_view_absl`, but some empty `string_view_absl`s are
// not null. Prefer checking for emptiness over checking for null.
//
// There are many ways to create an empty string_view_absl:
//
//   const char* nullcp = nullptr;
//   // string_view_absl.size() will return 0 in all cases.
//   absl::string_view_absl();
//   absl::string_view_absl(nullcp, 0);
//   absl::string_view_absl("");
//   absl::string_view_absl("", 0);
//   absl::string_view_absl("abcdef", 0);
//   absl::string_view_absl("abcdef" + 6, 0);
//
// All empty `string_view_absl` objects whether null or not, are equal:
//
//   absl::string_view_absl() == absl::string_view_absl("", 0)
//   absl::string_view_absl(nullptr, 0) == absl::string_view_absl("abcdef"+6, 0)
class string_view_absl {
 public:
  using traits_type = std::char_traits<char>;
  using value_type = char;
  using pointer = char*;
  using const_pointer = const char*;
  using reference = char&;
  using const_reference = const char&;
  using const_iterator = const char*;
  using iterator = const_iterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = const_reverse_iterator;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;

  static constexpr size_type npos = static_cast<size_type>(-1);

  // Null `string_view_absl` constructor
  constexpr string_view_absl() noexcept : ptr_(nullptr), length_(0) {}

  // Implicit constructors

  template <typename Allocator>
  string_view_absl(  // NOLINT(runtime/explicit)
      const std::basic_string<char, std::char_traits<char>, Allocator>&
          str) noexcept
      : ptr_(str.data()), length_(CheckLengthInternal(str.size())) {}

  // Implicit constructor of a `string_view_absl` from nul-terminated `str`. When
  // accepting possibly null strings, use `absl::NullSafeStringView(str)`
  // instead (see below).
#if ABSL_HAVE_BUILTIN(__builtin_strlen) || \
    (defined(__GNUC__) && !defined(__clang__))
  // GCC has __builtin_strlen according to
  // https://gcc.gnu.org/onlinedocs/gcc-4.7.0/gcc/Other-Builtins.html, but
  // ABSL_HAVE_BUILTIN doesn't detect that, so we use the extra checks above.
  // __builtin_strlen is constexpr.
  constexpr string_view_absl(const char* str)  // NOLINT(runtime/explicit)
      : ptr_(str),
        length_(CheckLengthInternal(str ? __builtin_strlen(str) : 0)) {}
#else
  constexpr string_view_absl(const char* str)  // NOLINT(runtime/explicit)
      : ptr_(str), length_(CheckLengthInternal(str ? strlen(str) : 0)) {}
#endif

  // Implicit constructor of a `string_view_absl` from a `const char*` and length.
  constexpr string_view_absl(const char* data, size_type len)
      : ptr_(data), length_(CheckLengthInternal(len)) {}

  // NOTE: Harmlessly omitted to work around gdb bug.
  //   constexpr string_view_absl(const string_view_absl&) noexcept = default;
  //   string_view_absl& operator=(const string_view_absl&) noexcept = default;

  // Iterators

  // string_view_absl::begin()
  //
  // Returns an iterator pointing to the first character at the beginning of the
  // `string_view_absl`, or `end()` if the `string_view_absl` is empty.
  constexpr const_iterator begin() const noexcept { return ptr_; }

  // string_view_absl::end()
  //
  // Returns an iterator pointing just beyond the last character at the end of
  // the `string_view_absl`. This iterator acts as a placeholder; attempting to
  // access it results in undefined behavior.
  constexpr const_iterator end() const noexcept { return ptr_ + length_; }

  // string_view_absl::cbegin()
  //
  // Returns a const iterator pointing to the first character at the beginning
  // of the `string_view_absl`, or `end()` if the `string_view_absl` is empty.
  constexpr const_iterator cbegin() const noexcept { return begin(); }

  // string_view_absl::cend()
  //
  // Returns a const iterator pointing just beyond the last character at the end
  // of the `string_view_absl`. This pointer acts as a placeholder; attempting to
  // access its element results in undefined behavior.
  constexpr const_iterator cend() const noexcept { return end(); }

  // string_view_absl::rbegin()
  //
  // Returns a reverse iterator pointing to the last character at the end of the
  // `string_view_absl`, or `rend()` if the `string_view_absl` is empty.
  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  // string_view_absl::rend()
  //
  // Returns a reverse iterator pointing just before the first character at the
  // beginning of the `string_view_absl`. This pointer acts as a placeholder;
  // attempting to access its element results in undefined behavior.
  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  // string_view_absl::crbegin()
  //
  // Returns a const reverse iterator pointing to the last character at the end
  // of the `string_view_absl`, or `crend()` if the `string_view_absl` is empty.
  const_reverse_iterator crbegin() const noexcept { return rbegin(); }

  // string_view_absl::crend()
  //
  // Returns a const reverse iterator pointing just before the first character
  // at the beginning of the `string_view_absl`. This pointer acts as a placeholder;
  // attempting to access its element results in undefined behavior.
  const_reverse_iterator crend() const noexcept { return rend(); }

  // Capacity Utilities

  // string_view_absl::size()
  //
  // Returns the number of characters in the `string_view_absl`.
  constexpr size_type size() const noexcept {
    return length_;
  }

  // string_view_absl::length()
  //
  // Returns the number of characters in the `string_view_absl`. Alias for `size()`.
  constexpr size_type length() const noexcept { return size(); }

  // string_view_absl::max_size()
  //
  // Returns the maximum number of characters the `string_view_absl` can hold.
  constexpr size_type max_size() const noexcept { return kMaxSize; }

  // string_view_absl::empty()
  //
  // Checks if the `string_view_absl` is empty (refers to no characters).
  constexpr bool empty() const noexcept { return length_ == 0; }

  // std::string:view::operator[]
  //
  // Returns the ith element of an `string_view_absl` using the array operator.
  // Note that this operator does not perform any bounds checking.
  constexpr const_reference operator[](size_type i) const { return ptr_[i]; }

  // string_view_absl::front()
  //
  // Returns the first element of a `string_view_absl`.
  constexpr const_reference front() const { return ptr_[0]; }

  // string_view_absl::back()
  //
  // Returns the last element of a `string_view_absl`.
  constexpr const_reference back() const { return ptr_[size() - 1]; }

  // string_view_absl::data()
  //
  // Returns a pointer to the underlying character array (which is of course
  // stored elsewhere). Note that `string_view_absl::data()` may contain embedded nul
  // characters, but the returned buffer may or may not be nul-terminated;
  // therefore, do not pass `data()` to a routine that expects a nul-terminated
  // std::string.
  constexpr const_pointer data() const noexcept { return ptr_; }

  // Modifiers

  // string_view_absl::remove_prefix()
  //
  // Removes the first `n` characters from the `string_view_absl`. Note that the
  // underlying std::string is not changed, only the view.
  void remove_prefix(size_type n) {
    assert(n <= length_);
    ptr_ += n;
    length_ -= n;
  }

  // string_view_absl::remove_suffix()
  //
  // Removes the last `n` characters from the `string_view_absl`. Note that the
  // underlying std::string is not changed, only the view.
  void remove_suffix(size_type n) {
    assert(n <= length_);
    length_ -= n;
  }

  // string_view_absl::swap()
  //
  // Swaps this `string_view_absl` with another `string_view_absl`.
  void swap(string_view_absl& s) noexcept {
    auto t = *this;
    *this = s;
    s = t;
  }

  // Explicit conversion operators

  // Converts to `std::basic_string`.
  template <typename A>
  explicit operator std::basic_string<char, traits_type, A>() const {
    if (!data()) return {};
    return std::basic_string<char, traits_type, A>(data(), size());
  }

  // string_view_absl::copy()
  //
  // Copies the contents of the `string_view_absl` at offset `pos` and length `n`
  // into `buf`.
  size_type copy(char* buf, size_type n, size_type pos = 0) const;

  // string_view_absl::substr()
  //
  // Returns a "substring" of the `string_view_absl` (at offset `pos` and length
  // `n`) as another string_view_absl. This function throws `std::out_of_bounds` if
  // `pos > size`.
  string_view_absl substr(size_type pos, size_type n = npos) const {
    if (ABSL_PREDICT_FALSE(pos > length_))
      base_internal::ThrowStdOutOfRange("absl::string_view_absl::substr");
    n = (std::min)(n, length_ - pos);
    return string_view_absl(ptr_ + pos, n);
  }

  // string_view_absl::compare()
  //
  // Performs a lexicographical comparison between the `string_view_absl` and
  // another `absl::string_view_absl`, returning -1 if `this` is less than, 0 if
  // `this` is equal to, and 1 if `this` is greater than the passed std::string
  // view. Note that in the case of data equality, a further comparison is made
  // on the respective sizes of the two `string_view_absl`s to determine which is
  // smaller, equal, or greater.
  int compare(string_view_absl x) const noexcept {
    auto min_length = (std::min)(length_, x.length_);
    if (min_length > 0) {
      int r = memcmp(ptr_, x.ptr_, min_length);
      if (r < 0) return -1;
      if (r > 0) return 1;
    }
    if (length_ < x.length_) return -1;
    if (length_ > x.length_) return 1;
    return 0;
  }

  // Overload of `string_view_absl::compare()` for comparing a substring of the
  // 'string_view_absl` and another `absl::string_view_absl`.
  int compare(size_type pos1, size_type count1, string_view_absl v) const {
    return substr(pos1, count1).compare(v);
  }

  // Overload of `string_view_absl::compare()` for comparing a substring of the
  // `string_view_absl` and a substring of another `absl::string_view_absl`.
  int compare(size_type pos1, size_type count1, string_view_absl v, size_type pos2,
              size_type count2) const {
    return substr(pos1, count1).compare(v.substr(pos2, count2));
  }

  // Overload of `string_view_absl::compare()` for comparing a `string_view_absl` and a
  // a different  C-style std::string `s`.
  int compare(const char* s) const { return compare(string_view_absl(s)); }

  // Overload of `string_view_absl::compare()` for comparing a substring of the
  // `string_view_absl` and a different std::string C-style std::string `s`.
  int compare(size_type pos1, size_type count1, const char* s) const {
    return substr(pos1, count1).compare(string_view_absl(s));
  }

  // Overload of `string_view_absl::compare()` for comparing a substring of the
  // `string_view_absl` and a substring of a different C-style std::string `s`.
  int compare(size_type pos1, size_type count1, const char* s,
              size_type count2) const {
    return substr(pos1, count1).compare(string_view_absl(s, count2));
  }

  // Find Utilities

  // string_view_absl::find()
  //
  // Finds the first occurrence of the substring `s` within the `string_view_absl`,
  // returning the position of the first character's match, or `npos` if no
  // match was found.
  size_type find(string_view_absl s, size_type pos = 0) const noexcept;

  // Overload of `string_view_absl::find()` for finding the given character `c`
  // within the `string_view_absl`.
  size_type find(char c, size_type pos = 0) const noexcept;

  // string_view_absl::rfind()
  //
  // Finds the last occurrence of a substring `s` within the `string_view_absl`,
  // returning the position of the first character's match, or `npos` if no
  // match was found.
  size_type rfind(string_view_absl s, size_type pos = npos) const
      noexcept;

  // Overload of `string_view_absl::rfind()` for finding the last given character `c`
  // within the `string_view_absl`.
  size_type rfind(char c, size_type pos = npos) const noexcept;

  // string_view_absl::find_first_of()
  //
  // Finds the first occurrence of any of the characters in `s` within the
  // `string_view_absl`, returning the start position of the match, or `npos` if no
  // match was found.
  size_type find_first_of(string_view_absl s, size_type pos = 0) const
      noexcept;

  // Overload of `string_view_absl::find_first_of()` for finding a character `c`
  // within the `string_view_absl`.
  size_type find_first_of(char c, size_type pos = 0) const
      noexcept {
    return find(c, pos);
  }

  // string_view_absl::find_last_of()
  //
  // Finds the last occurrence of any of the characters in `s` within the
  // `string_view_absl`, returning the start position of the match, or `npos` if no
  // match was found.
  size_type find_last_of(string_view_absl s, size_type pos = npos) const
      noexcept;

  // Overload of `string_view_absl::find_last_of()` for finding a character `c`
  // within the `string_view_absl`.
  size_type find_last_of(char c, size_type pos = npos) const
      noexcept {
    return rfind(c, pos);
  }

  // string_view_absl::find_first_not_of()
  //
  // Finds the first occurrence of any of the characters not in `s` within the
  // `string_view_absl`, returning the start position of the first non-match, or
  // `npos` if no non-match was found.
  size_type find_first_not_of(string_view_absl s, size_type pos = 0) const noexcept;

  // Overload of `string_view_absl::find_first_not_of()` for finding a character
  // that is not `c` within the `string_view_absl`.
  size_type find_first_not_of(char c, size_type pos = 0) const noexcept;

  // string_view_absl::find_last_not_of()
  //
  // Finds the last occurrence of any of the characters not in `s` within the
  // `string_view_absl`, returning the start position of the last non-match, or
  // `npos` if no non-match was found.
  size_type find_last_not_of(string_view_absl s,
                                          size_type pos = npos) const noexcept;

  // Overload of `string_view_absl::find_last_not_of()` for finding a character
  // that is not `c` within the `string_view_absl`.
  size_type find_last_not_of(char c, size_type pos = npos) const
      noexcept;

 private:
  static constexpr size_type kMaxSize =
      (std::numeric_limits<difference_type>::max)();

  static constexpr size_type CheckLengthInternal(size_type len) {
    return ABSL_ASSERT(len <= kMaxSize), len;
  }

  const char* ptr_;
  size_type length_;
};

// This large function is defined inline so that in a fairly common case where
// one of the arguments is a literal, the compiler can elide a lot of the
// following comparisons.
inline bool operator==(string_view_absl x, string_view_absl y) noexcept {
  auto len = x.size();
  if (len != y.size()) {
    return false;
  }
  return x.data() == y.data() || len <= 0 ||
         memcmp(x.data(), y.data(), len) == 0;
}

inline bool operator!=(string_view_absl x, string_view_absl y) noexcept {
  return !(x == y);
}

inline bool operator<(string_view_absl x, string_view_absl y) noexcept {
  auto min_size = (std::min)(x.size(), y.size());
  const int r = min_size == 0 ? 0 : memcmp(x.data(), y.data(), min_size);
  return (r < 0) || (r == 0 && x.size() < y.size());
}

inline bool operator>(string_view_absl x, string_view_absl y) noexcept { return y < x; }

inline bool operator<=(string_view_absl x, string_view_absl y) noexcept {
  return !(y < x);
}

inline bool operator>=(string_view_absl x, string_view_absl y) noexcept {
  return !(x < y);
}

// IO Insertion Operator
std::ostream& operator<<(std::ostream& o, string_view_absl piece);

}  // namespace absl

#endif  // ABSL_HAVE_STD_string_view_absl

namespace absl {

// ClippedSubstr()
//
// Like `s.substr(pos, n)`, but clips `pos` to an upper bound of `s.size()`.
// Provided because std::string_view_absl::substr throws if `pos > size()`
inline string_view_absl ClippedSubstr(string_view_absl s, size_t pos,
                                 size_t n = string_view_absl::npos) {
  pos = (std::min)(pos, static_cast<size_t>(s.size()));
  return s.substr(pos, n);
}

// NullSafeStringView()
//
// Creates an `absl::string_view_absl` from a pointer `p` even if it's null-valued.
// This function should be used where an `absl::string_view_absl` can be created from
// a possibly-null pointer.
inline string_view_absl NullSafeStringView(const char* p) {
  return p ? string_view_absl(p) : string_view_absl();
}

}  // namespace absl

#endif  // ABSL_STRINGS_string_view_absl_H_
