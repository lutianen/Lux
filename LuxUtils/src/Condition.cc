/**
 * @file condition.cc
 * @brief
 *
 * @author Tianen Lu (tianenlu957@gmail.com)
 */

#include <LuxUtils/Condition.h>
#include <errno.h>

bool Lux::Condition::waitForSeconds(double seconds) {
    struct timespec abstime;

    // FIXME: use CLOCK_MONOTONIC or CLOCK_MONOTONIC_RAW to prevent time rewind.
    ::clock_gettime(CLOCK_REALTIME, &abstime);

    const int64_t kNanoSecondsPerSecond = 1000000000;
    int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);

    abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) /
                                          kNanoSecondsPerSecond);
    abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) %
                                        kNanoSecondsPerSecond);

    MutexLock::UnassignGuard ug(mutex_);
    return ETIMEDOUT == ::pthread_cond_timedwait(
                            &pcond_, mutex_.getPthreadMutex(), &abstime);
}
