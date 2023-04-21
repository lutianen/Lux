/**
 * @file Timer.h
 * @brief
 *
 * @author Lux
 */

#pragma once

#include <LuxUtils/Atomic.h>
#include <LuxUtils/Timestamp.h>
#include <polaris/Callbacks.h>

namespace Lux {
namespace polaris {

/// @brief Internal class for timer event.
class Timer {
    Timer(const Timer&) = delete;
    Timer& operator=(Timer&) = delete;

public:
    Timer(TimerCallback cb, Timestamp when, double interval)
        : callback_(std::move(cb)),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0),
          sequence_(s_numCreated_.incrementAndGet()) {}

    void run() const { callback_(); }

    inline Timestamp expiration() const { return expiration_; }
    inline bool repeat() const { return repeat_; }
    inline int64_t sequence() const { return sequence_; }

    void restart(Timestamp now);

    static inline int64_t numCreated() { return s_numCreated_.get(); }

private:
    const TimerCallback callback_;
    Timestamp expiration_;

    const double interval_;
    const bool repeat_;
    const int64_t sequence_;

    static AtomicInt64 s_numCreated_;
};

}  // namespace polaris
}  // namespace Lux
