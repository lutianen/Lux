/**
 * @file Callbacks.h
 * @brief
 *
 * @version 1.0
 * @author Lux
 * @date 2022-11
 */

#pragma once

#include <LuxUtils/Timestamp.h>

#include <functional>
#include <memory>

namespace Lux {
template <typename T>
inline T* get_pointer(const std::shared_ptr<T>& ptr) {
    return ptr.get();
}

template <typename T>
inline T* get_pointer(const std::unique_ptr<T>& ptr) {
    return ptr.get();
}

// Adapted from google-protobuf stubs/common.h
template <typename To, typename From>
inline ::std::shared_ptr<To> down_pointer_cast(
    const ::std::shared_ptr<From>& f) {
    if (false) {
        implicit_cast<From*, To*>(0);
    }

#ifndef NDEBUG
    assert(f == NULL || dynamic_cast<To*>(get_pointer(f)) != NULL);
#endif
    return ::std::static_pointer_cast<To>(f);
}

namespace polaris {
// All client visible callbacks go here.

class Buffer;
class TCPConnection;
typedef std::shared_ptr<TCPConnection> TCPConnectionPtr;
typedef std::function<void()> TimerCallback;
typedef std::function<void(const TCPConnectionPtr&)> ConnectionCallback;
typedef std::function<void(const TCPConnectionPtr&)> CloseCallback;
typedef std::function<void(const TCPConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void(const TCPConnectionPtr&, size_t)>
    HighWaterMarkCallback;

// the data has been read to (buf, len)
typedef std::function<void(const TCPConnectionPtr&, Buffer*, Timestamp)>
    MessageCallback;

void defaultConnectionCallback(const TCPConnectionPtr& conn);
void defaultMessageCallback(const TCPConnectionPtr& conn, Buffer* buffer,
                            Timestamp receiveTime);

}  // namespace polaris

template <typename CLASS, typename... ARGS>
class WeakCallback {
public:
    WeakCallback(const std::weak_ptr<CLASS>& object,
                 const std::function<void(CLASS*, ARGS...)>& function)
        : object_(object), function_(function) {}

    /// Default dtor, copy ctor and assignment are okay

    /// @brief
    /// @param ...args
    inline void operator()(ARGS&&... args) const {
        std::shared_ptr<CLASS> ptr(object_.lock());
        if (ptr) {
            function_(ptr.get(), std::forward<ARGS>(args)...);
        }
        // else
        // {
        //   LOG_TRACE << "expired";
        // }
    }

private:
    std::weak_ptr<CLASS> object_;
    std::function<void(CLASS*, ARGS...)> function_;
};

template <typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> makeWeakCallback(
    const std::shared_ptr<CLASS>& object, void (CLASS::*function)(ARGS...)) {
    return WeakCallback<CLASS, ARGS...>(object, function);
}

template <typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> makeWeakCallback(
    const std::shared_ptr<CLASS>& object,
    void (CLASS::*function)(ARGS...) const) {
    return WeakCallback<CLASS, ARGS...>(object, function);
}
}  // namespace Lux
