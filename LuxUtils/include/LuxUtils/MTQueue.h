/**
 * @file MTQueue.h
 * @brief 线程安全的生产者-消费者队列
 *
 * @author Lux
 */

#include <algorithm>
#include <condition_variable>
#include <initializer_list>
#include <mutex>
#include <vector>

/**
 * @brief 线程安全的生产者-消费者队列
 *
 * @tparam T
 */
template <typename T>
class MTQueue {
    std::condition_variable cv_;
    std::mutex mutex_;
    std::vector<T> que_;

public:
    T pop() {
#if __cplusplus >= 201703
        std::unique_lock lock(mutex_);
#else
        std::unique_lock<std::mutex> lock(mutex_);
#endif

        cv_.wait(lock, [this] { return !que_.empty(); });

        T ret = std::move(que_.back());
        que_.pop_back();
        return ret;
    }

    auto popHead() {
#if __cplusplus >= 201703
        std::unique_lock lock(mutex_);
#else
        std::unique_lock<std::mutex> lock(mutex_);
#endif
        cv_.wait(lock, [this] { return !que_.empty(); });

        T ret = std::move(que_.back());
        que_.pop_back();

#if __cplusplus >= 201703
        return std::pair(std::move(ret), std::move(lock));
#else
        return std::pair<decltype(ret), decltype(lock)>(std::move(ret),
                                                        std::move(lock));
#endif
    }

    void push(T val) {
#if __cplusplus >= 201703
        std::unique_lock lock(mutex_);
#else
        std::unique_lock<std::mutex> lock(mutex_);
#endif
        que_.push_back(std::move(val));
        cv_.notify_one();
    }

    void pushMany(std::initializer_list<T> vals) {
#if __cplusplus >= 201703
        std::unique_lock lock(mutex_);
        std::copy(std::move_iterator(vals.begin()),
                  std::move_iterator(vals.end()), std::back_inserter(que_));
#else
        std::unique_lock<std::mutex> lock(mutex_);
        std::copy(std::move_iterator<decltype(vals.begin())>(vals.begin()),
                  std::move_iterator<decltype(vals.begin())>(vals.end()),
                  std::back_insert_iterator<std::vector<T> >(que_));
#endif
        cv_.notify_all();
    }
};
