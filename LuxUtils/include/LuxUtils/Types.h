/**
 * @file types.h
 * @brief memZero implicit_cast down_cast
 *
 * @author Tianen Lu (tianenlu957@gmail.com)
 */

#pragma once

#include <cstdint>
#include <cstring>  // memset
#include <functional>
#include <string>

#ifndef NDEBUG
#include <cassert>  // assert
#endif

///
/// The most common stuffs.
///
namespace Lux {
using std::string;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

/// @brief Set N bytes of S to zero.
/// @param S void*
/// @param N size_t
inline void memZero(void* S, size_t N) { ::memset(S, 0, N); }

template <typename To, typename From>
inline To implicit_cast(From const& f) {
    return f;
}

template <typename To, typename From>  // use like this: down_cast<T*>(foo);
inline To                              // so we only accept pointers
down_cast(From* f) {
    // Ensures that To is a sub-type of From *.  This test is here only
    // for compile-time type checking, and has no overhead in an
    // optimized build at run-time, as it will be optimized away
    // completely.
    if (false) {
        implicit_cast<From*, To>(0);
    }

#if !defined(NDEBUG) && !defined(GOOGLE_PROTOBUF_NO_RTTI)
    assert(f == NULL || dynamic_cast<To>(f) != NULL);  // RTTI: debug mode only!
#endif

    return static_cast<To>(f);
}

}  // namespace Lux
