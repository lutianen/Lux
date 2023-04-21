/**
 * @file Timestamp.h
 * @brief
 *
 * @author Tianen Lu (tianenlu957@gmail.com)
 */

#pragma once

#include <LuxUtils/Types.h>

namespace Lux {

/**
 * @brief Time stamp in LOCAL TIME, in micro seconds resolution.
 *
 * This class is immutable.
 * It's recommended to pass it by value, since it's passed in register on x64.
 */
class Timestamp {
private:
    /// @brief the number of micro seconds since the Epoch, 1970-01-01 00:00:00
    /// +0000 (UTC). (微秒)
    int64_t microSecondsSinceEpoch_;

public:
    static const int kMicroSecondsPerSecond = 1000 * 1000;

    /// @brief Constucts an invalid Timestamp.
    Timestamp() : microSecondsSinceEpoch_(0) {}

    /// @brief Constucts a Timestamp at specific time
    /// @param secondsSinceEpochArg
    explicit Timestamp(int64_t microSecondsSinceEpochArg)
        : microSecondsSinceEpoch_(microSecondsSinceEpochArg) {}

    /// @brief Swap timestamp with @c that
    /// @param that
    inline void swap(Timestamp& that) {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
    }

    // default copy/assignment/dtor are Okay

    string toString() const;
    string toFormattedString(bool showMicroSecond = false) const;

    /// @brief valid - validate Timestamp
    /// @return microSecondsSinceEpoch_ > 0 ? ture : false
    inline bool valid() const { return microSecondsSinceEpoch_ > 0; }

    /// @brief for internal usage.
    /// @return
    inline int64_t microSecondsSinceEpoch() const {
        return microSecondsSinceEpoch_;
    }
    time_t secondsSinceEpoch() const {
        return static_cast<time_t>(microSecondsSinceEpoch_ /
                                   kMicroSecondsPerSecond);
    }

    ///
    /// Get time of now.
    ///
    static Timestamp now();
    static Timestamp invalid() { return Timestamp(); }

    static inline Timestamp fromUnixTime(time_t t) {
        return fromUnixTime(t, 0);
    }

    static inline Timestamp fromUnixTime(time_t t, int microseconds) {
        return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond +
                         microseconds);
    }
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

///
/// Gets time difference of two timestamps, result in seconds.
///
/// @param high, low
/// @return (high-low) in seconds
inline double timeDifference(Timestamp high, Timestamp low) {
    return static_cast<double>(high.microSecondsSinceEpoch() -
                               low.microSecondsSinceEpoch()) /
           Timestamp::kMicroSecondsPerSecond;
}

///
/// Add @c seconds to given timestamp.
///
/// @return timestamp+seconds as Timestamp
///
inline Timestamp addTime(Timestamp timestamp, double seconds) {
    auto delta =
        static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}
}  // namespace Lux
