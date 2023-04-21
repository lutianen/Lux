/**
 * @file CountDownLatch.h
 * @brief pthread_join:
 *              是只要线程 active
 * 就会阻塞，线程结束就会返回，一般用于主线程回收工作线程 CountDownLatch:
 *              可以保证工作线程的任务执行完毕，主线程再对工作线程进行回收
 *              本质上来说,是一个thread safe的计数器,用于主线程和工作线程的同步
 *              用法：
 *                1. 在初始化时,需要指定主线程需要等待的任务的个数(count),
 *                   当工作线程完成 Task
 * Callback后对计数器减1，而主线程通过wait()调用 阻塞等待技术器减到0为止.
 *                2.
 * 初始化计数器值为1,在程序结尾将创建一个线程执行CountDown操作并wait()，
 *                   当程序执行到最后会阻塞直到计数器减为0,这可以保证线程池中的线程都
 *                   start了线程池对象才完成析构,实现ThreadPool的过程中遇到过
 *              核心函数：
 *                1. countDwon()
 *                   对计数器进行原子减一操作
 *                2. wait()
 *                   使用条件变量等待计数器减到零，然后notify
 *
 * @author Tianen Lu
 */

#pragma once

#include <LuxUtils/Condition.h>
#include <LuxUtils/Mutex.h>

namespace Lux {
class CountDownLatch {
    CountDownLatch(const CountDownLatch&) = delete;
    CountDownLatch& operator=(CountDownLatch&) = delete;

private:
    mutable MutexLock mutex_;
    Condition condition_ GUARDED_BY(mutex_);
    int count_ GUARDED_BY(mutex_);

public:
    explicit CountDownLatch(int count);

    /// @brief 使用条件变量等待计数器减到零，然后notify
    void wait();

    /// @brief 对计数器进行原子减一操作
    void countDown();

    /// @brief 获取计数器值
    /// @return int
    int getCount() const;
};
}  // namespace Lux