/**
 * @file Condition.h
 * @brief
 * Cond(条件变量)，与互斥量一起使用，允许线程以无竞争的方式等待特定的条件发生.
 *  条件本身是由互斥量保护的
 *  线程在改变条件状态之前必须首先锁住互斥量
 *  条件变量使用 pthread_cond_t 数据类型表示
 *  把 PTHREAD_COND_INITIALIZER 赋值给静态分配的条件变量进行初始化
 *  使用 int pthread_cond_init(pthread_cond_t *cond,
 *       const pthread_condattr_t *restrict attr) 对动态分配的条件变量进行初始化
 *  使用 int pthread_cond_destroy(pthread_cond_t *cond) 进行反初始化
 *  使用 int pthread_cond_wait
 * 等待条件变量变为真，如果在给定的时间内条件不能满足，则会生成一个返回错误码的变量
 *  pthread_cond_signal 至少唤醒一个等待该条件的线程
 *  pthread_cond_broadcast 唤醒等待该条件的所有线程
 *
 * @author Tianen Lu (tianenlu957@gmail.com)
 */

#pragma once

#include <LuxUtils/Mutex.h>

namespace Lux {
/**
 * pthread 中 condition 的核心函数：创建、销毁、等待、通知、广播
 *  pthread_cond_init
 *  pthread_cond_destroy
 *  pthread_cond_wait
 *  pthread_cond_signal
 *  pthread_cond_broadcast
 */
class Condition {
    Condition(const Condition&) = delete;
    Condition& operator=(Condition&) = delete;

private:
    /// @brief 条件变量 通常和 互斥量 一起使用
    MutexLock& mutex_;
    pthread_cond_t pcond_;

public:
    /// @brief 对动态分配的条件变量进行初始化，使用默认属性
    /// @param mutex
    explicit Condition(MutexLock& mutex) : mutex_(mutex) {
        MCHECK(pthread_cond_init(&pcond_, nullptr));
    }

    /// 对条件变量进行反初始化
    ~Condition() { MCHECK(pthread_cond_destroy(&pcond_)); }

    /// @brief 等待条件变量变为真
    inline void wait() {
        MutexLock::UnassignGuard ug(mutex_);
        MCHECK(pthread_cond_wait(&pcond_, mutex_.getPthreadMutex()));
    }

    /// @brief returns true if time out, false otherwise.
    bool waitForSeconds(double seconds);

    /// @brief 至少唤醒一个等待该条件的线程
    inline void notify() { MCHECK(pthread_cond_signal(&pcond_)); }

    /// @brief 唤醒等待该条件的所有线程
    inline void notifyAll() { MCHECK(pthread_cond_broadcast(&pcond_)); }
};
}  // namespace Lux
