/**
 * @file CurrentThread.h
 * @brief
 *
 * @author Tianen Lu
 */

#pragma once

#include <LuxUtils/Types.h>

namespace Lux {
namespace CurrentThread {

// internal
extern __thread int t_cachedTid;  // 用来缓存 pid
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;
extern __thread const char* t_threadName;

// impl in Thread.cc
void cacheTid();

inline int tid() {
    if (__builtin_expect(t_cachedTid == 0, 0)) {
        cacheTid();
    }
    return t_cachedTid;
}

// for logging
inline const char* tidString() { return t_tidString; }
// for logging
inline int tidStringLength() { return t_tidStringLength; }

inline const char* name() { return t_threadName; }

// impl in Thread.cc
bool isMainThread();
void sleepUsec(int64_t usec);  // for testing

string stackTrace(bool demangle);

}  // namespace CurrentThread

}  // namespace Lux
