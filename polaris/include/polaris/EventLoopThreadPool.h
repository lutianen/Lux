/**
 * @file EventLoopThreadPool.h
 * @brief
 *
 * @author Lux
 */

#pragma once

#include <LuxLog/Logger.h>
#include <LuxUtils/Types.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Lux {
namespace polaris {

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool {
    EventLoopThreadPool(const EventLoopThreadPool&) = delete;
    EventLoopThreadPool& operator=(EventLoopThreadPool&) = delete;

public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

private:
    EventLoop* baseLoop_;
    string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;

public:
    EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg);
    ~EventLoopThreadPool();

    inline void setThreadNum(int numThreads) {
        LOG_DEBUG << "numThreads: " << numThreads_;
        numThreads_ = numThreads;
    }
    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    /// @brief valid after calling start()
    /// round-robin
    /// @return EventLoop*
    EventLoop* getNextLoop();

    /// @brief with the same hash code, it will always return the same
    /// EventLoop
    /// @param hashCOde
    /// @return EventLoop*
    EventLoop* getLoopForHash(size_t hashCOde);

    std::vector<EventLoop*> getAllLoops();

    inline bool started() const { return started_; }

    inline const string& name() const { return name_; }
};
}  // namespace polaris
}  // namespace Lux
