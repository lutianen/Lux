/**
 * @file EventLoopThread.h
 * @brief
 *
 * @author Lux
 */

#pragma once

#include <LuxUtils/Condition.h>
#include <LuxUtils/Mutex.h>
#include <LuxUtils/Thread.h>

namespace Lux {
namespace polaris {
class EventLoop;

class EventLoopThread {
    EventLoopThread(const EventLoopThread&) = delete;
    EventLoopThread& operator=(EventLoopThread&) = delete;

public:
    using ThreadInitCallback = std::function<void(EventLoop* loop)>;

private:
    EventLoop* loop_ GUARDED_BY(mutex_);
    bool exiting_;
    Thread thread_;

    MutexLock mutex_;
    Condition cond_ GUARDED_BY(mutex_);

    ThreadInitCallback callback_;

private:
    void threadFunc();

public:
    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                    const std::string& name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();
};
}  // namespace polaris
}  // namespace Lux
