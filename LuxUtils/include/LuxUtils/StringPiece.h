/**
 * @file StringPiece.h
 * @brief
 *
 * @author Tianen Lu
 */

#pragma once

// Taken from PCRE pcre_stringpiece.h
//
// Copyright (c) 2005, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Sanjay Ghemawat
//
// A string like object that points into another piece of memory.
// Useful for providing an interface that allows clients to easily
// pass in either a "const char*" or a "string".
//
// Arghh!  I wish C++ literals were automatically of type "string".

#include <LuxUtils/Types.h>

#include <iosfwd>  // for ostream forward-declaration

namespace Lux {

/// @brief For passing C-style string argument to a function.
/// copyable
class StringArg {
private:
    /// @brief Core data member
    const char* str_;

public:
    StringArg(const char* str) : str_(str) {}

    StringArg(const string& str) : str_(str.c_str()) {}

    /// @brief Return const pointer to null-terminated contents.
    /// 返回指向以 null 结尾的内容的 const 指针
    const char* c_str() const { return str_; }
};

/// C++ 17
#if __cplusplus >= 201703L
#include <string_view>

typedef std::string_view StringPiece;

/// C++ 11
#else
/**
 * @brief A class dedicated to passing string parameters by Google.
 * Don't need to provide the two overload versions: const char*, const
 * std::string&
 */
class StringPiece {
private:
    const char* ptr_;
    int length_;

public:
    // We provide non-explicit singleton constructors so users can pass
    // in a "const char*" or a "string" wherever a "StringPiece" is
    // expected.
    StringPiece() : ptr_(NULL), length_(0) {}
    StringPiece(const char* str)
        : ptr_(str), length_(static_cast<int>(strlen(ptr_))) {}
    StringPiece(const unsigned char* str)
        : ptr_(reinterpret_cast<const char*>(str)),
          length_(static_cast<int>(strlen(ptr_))) {}
    StringPiece(const std::string& str)
        : ptr_(str.data()), length_(static_cast<int>(str.size())) {}
    StringPiece(const char* offset, int len) : ptr_(offset), length_(len) {}

    // data() may return a pointer to a buffer with embedded NULs, and the
    // returned buffer may or may not be null terminated.  Therefore it is
    // typically a mistake to pass data() to a routine that expects a NUL
    // terminated string.  Use "as_string().c_str()" if you really need to do
    // this.  Or better yet, change your routine so it does not rely on NUL
    // termination.
    inline const char* data() const { return ptr_; }
    inline int size() const { return length_; }
    inline bool empty() const { return length_ == 0; }
    inline const char* begin() const { return ptr_; }
    inline const char* end() const { return ptr_ + length_; }
    inline void clear() {
        ptr_ = NULL;
        length_ = 0;
    }

    inline void set(const char* buffer, int len) {
        ptr_ = buffer;
        length_ = len;
    }
    inline void set(const char* str) {
        ptr_ = str;
        length_ = static_cast<int>(strlen(str));
    }
    inline void set(const void* buffer, int len) {
        ptr_ = reinterpret_cast<const char*>(buffer);
        length_ = len;
    }

    inline char operator[](int i) const { return ptr_[i]; }

    inline void remove_prefix(int n) {
        ptr_ += n;
        length_ -= n;
    }

    inline void remove_suffix(int n) { length_ -= n; }

    inline bool operator==(const StringPiece& x) const {
        return ((length_ == x.length_) &&
                (::memcmp(ptr_, x.ptr_, static_cast<size_t>(length_)) == 0));
    }
    inline bool operator!=(const StringPiece& x) const { return !(*this == x); }

#define STRINGPIECE_BINARY_PREDICATE(cmp, auxcmp)                           \
    inline bool operator cmp(const StringPiece& x) const {                  \
        int r =                                                             \
            ::memcmp(ptr_, x.ptr_,                                          \
                     length_ < x.length_ ? static_cast<size_t>(length_)     \
                                         : static_cast<size_t>(x.length_)); \
        return ((r auxcmp 0) || ((r == 0) && (length_ cmp x.length_)));     \
    }

    STRINGPIECE_BINARY_PREDICATE(<, <);
    STRINGPIECE_BINARY_PREDICATE(<=, <);
    STRINGPIECE_BINARY_PREDICATE(>=, >);
    STRINGPIECE_BINARY_PREDICATE(>, >);
#undef STRINGPIECE_BINARY_PREDICATE

    inline int compare(const StringPiece& x) const {
        int r = ::memcmp(ptr_, x.ptr_,
                         length_ < x.length_ ? static_cast<size_t>(length_)
                                             : static_cast<size_t>(x.length_));
        if (r == 0) {
            if (length_ < x.length_)
                r = -1;
            else if (length_ > x.length_)
                r = +1;
        }
        return r;
    }

    inline std::string as_string() const {
        return std::string(
            data(), static_cast<std::basic_string<char>::size_type>(size()));
    }

    inline void CopyToString(std::string* target) const {
        target->assign(
            ptr_, static_cast<std::basic_string<char>::size_type>(length_));
    }

    // Does "this" start with "x"
    inline bool starts_with(const StringPiece& x) const {
        return ((length_ >= x.length_) &&
                (::memcmp(ptr_, x.ptr_, static_cast<size_t>(x.length_)) == 0));
    }
};

#endif

}  // namespace Lux

// ------------------------------------------------------------------
// Functions used to create STL containers that use StringPiece
//  Remember that a StringPiece's lifetime had better be less than
//  that of the underlying string or char*.  If it is not, then you
//  cannot safely store a StringPiece into an STL container
// ------------------------------------------------------------------

#ifdef HAVE_TYPE_TRAITS
// This makes vector<StringPiece> really fast for some STL implementations
template <>
struct __type_traits<muduo::StringPiece> {
    typedef __true_type has_trivial_default_constructor;
    typedef __true_type has_trivial_copy_constructor;
    typedef __true_type has_trivial_assignment_operator;
    typedef __true_type has_trivial_destructor;
    typedef __true_type is_POD_type;
};
#endif

/// @brief Allow StringPiece to be logged
/// @param o std::ostream&
/// @param piece Lux::StringPiece&
/// @return std::ostream&
std::ostream& operator<<(std::ostream& o, const Lux::StringPiece& piece);
