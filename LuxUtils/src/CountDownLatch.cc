/**
 * @file CountDownLatch.cc
 * @brief
 *
 * @author Tianen Lu (tianenlu957@gmail.com)
 */

#include <LuxUtils/CountDownLatch.h>

Lux::CountDownLatch::CountDownLatch(int count)
    : mutex_(), condition_(mutex_), count_(count) {}

void Lux::CountDownLatch::wait() {
    Lux::MutexLockGuard lock(mutex_);
    while (count_ > 0) {
        condition_.wait();
    }
}

void Lux::CountDownLatch::countDown() {
    MutexLockGuard lock(mutex_);
    --count_;
    if (count_ == 0) {
        condition_.notifyAll();
    }
}

int Lux::CountDownLatch::getCount() const {
    MutexLockGuard lock(mutex_);
    return count_;
}
