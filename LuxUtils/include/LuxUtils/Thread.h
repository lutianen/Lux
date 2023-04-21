/**
 * @file Thread.h
 * @brief 核心函数 start  join
 *      start 调用线程函数
 *      join 等待主线程回收
 *
 * @author Tianen Lu
 */

#pragma once

#include <LuxUtils/Atomic.h>
#include <LuxUtils/CountDownLatch.h>
#include <pthread.h>  // pthread_t

#include <functional>  // function
#include <memory>

namespace Lux {
/**
 * pthread 核心函数：线程的创建和等待结束
 * pthread_create
 * pthread_join
 */
class Thread {
    Thread(const Thread&) = delete;
    Thread& operator=(Thread&) = delete;

public:
    using ThreadFunc = std::function<void()>;

private:
    void setDefaultName();

    bool started_;
    bool joined_;
    /* pthread_t 不适合做程序中对线程的标识符 */
    pthread_t pthreadId_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    CountDownLatch latch_;

    static AtomicInt32 numCreated_;

public:
    explicit Thread(ThreadFunc, std::string name = std::string());
    // FIXME: make it movable in C++11
    ~Thread();

    void start();
    int join();  // return pthread_join()

    bool started() const { return started_; }

    // pthread_t
    // pthreadId() const { return pthreadId_; }

    pid_t tid() const { return tid_; }

    const std::string& name() const { return name_; }

    static int numCreated() { return numCreated_.get(); }
};
}  // namespace Lux
