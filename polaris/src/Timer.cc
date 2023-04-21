/**
 * @file Timer.cc
 * @brief
 *
 * @author Lux
 */

#include <polaris/Timer.h>

using namespace Lux;
using namespace Lux::polaris;

AtomicInt64 Timer::s_numCreated_;

void Timer::restart(Timestamp now) {
    if (repeat_) {
        expiration_ = addTime(now, interval_);
    } else {
        expiration_ = Timestamp::invalid();
    }
}
