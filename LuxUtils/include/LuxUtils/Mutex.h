/**
 * @file Mutex.h
 * @brief mutex 互斥量，本质上是一把锁。
 *  在访问共享资源前对互斥量进行设置（加锁），访问完成后释放（解锁）互斥量。
 *  互斥量使用 pthread_mutex_t 数据类型表示
 *  调用 pthread_mutex_init 函数进行初始化
 *  对互斥量加锁调用 int pthread_mutex_lock(pthread_mutex_t *mutex)
 *  对互斥量解锁锁调用 int pthread_mutex_unlock(pthread_mutex_t *mutex)
 *
 * @author Tianen Lu
 */

#pragma once

#include <LuxUtils/CurrentThread.h>
#include <pthread.h>

#include <cassert>

// Thread safety annotations {
// https://clang.llvm.org/docs/ThreadSafetyAnalysis.html
// Enable thread safety attributes only with clang.
// The attributes can be safely erased when compiling with other compilers.
#if defined(__clang__) && (!defined(SWIG))
#define THREAD_ANNOTATION_ATTRIBUTE__(x) __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x)  // no-op
#endif

#define CAPABILITY(x) THREAD_ANNOTATION_ATTRIBUTE__(capability(x))

#define SCOPED_CAPABILITY THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)

#define GUARDED_BY(x) THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

#define PT_GUARDED_BY(x) THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_by(x))

#define ACQUIRED_BEFORE(...) \
    THREAD_ANNOTATION_ATTRIBUTE__(acquired_before(__VA_ARGS__))

#define ACQUIRED_AFTER(...) \
    THREAD_ANNOTATION_ATTRIBUTE__(acquired_after(__VA_ARGS__))

#define REQUIRES(...) \
    THREAD_ANNOTATION_ATTRIBUTE__(requires_capability(__VA_ARGS__))

#define REQUIRES_SHARED(...) \
    THREAD_ANNOTATION_ATTRIBUTE__(requires_shared_capability(__VA_ARGS__))

#define ACQUIRE(...) \
    THREAD_ANNOTATION_ATTRIBUTE__(acquire_capability(__VA_ARGS__))

#define ACQUIRE_SHARED(...) \
    THREAD_ANNOTATION_ATTRIBUTE__(acquire_shared_capability(__VA_ARGS__))

#define RELEASE(...) \
    THREAD_ANNOTATION_ATTRIBUTE__(release_capability(__VA_ARGS__))

#define RELEASE_SHARED(...) \
    THREAD_ANNOTATION_ATTRIBUTE__(release_shared_capability(__VA_ARGS__))

#define TRY_ACQUIRE(...) \
    THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_capability(__VA_ARGS__))

#define TRY_ACQUIRE_SHARED(...) \
    THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_shared_capability(__VA_ARGS__))

#define EXCLUDES(...) THREAD_ANNOTATION_ATTRIBUTE__(locks_excluded(__VA_ARGS__))

#define ASSERT_CAPABILITY(x) THREAD_ANNOTATION_ATTRIBUTE__(assert_capability(x))

#define ASSERT_SHARED_CAPABILITY(x) \
    THREAD_ANNOTATION_ATTRIBUTE__(assert_shared_capability(x))

#define RETURN_CAPABILITY(x) THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(x))

#define NO_THREAD_SAFETY_ANALYSIS \
    THREAD_ANNOTATION_ATTRIBUTE__(no_thread_safety_analysis)
// End of thread safety annotations }3

#ifdef CHECK_PTHREAD_RETURN_VALUE

#ifdef NDEBUG
__BEGIN_DECLS
extern void __assert_perror_fail(int errnum, const char* file,
                                 unsigned int line,
                                 const char* function) noexcept
    __attribute__((__noreturn__));
__END_DECLS
#endif

/**
 * @brief __builtin_expect(EXP, N)
 * 这个指令是gcc引入的，作用是允许程序员将最有可能执行的分支告诉编译器。
 * 意思是：EXP==N的概率很大。
 */
#define MCHECK(ret)                                                     \
    ({                                                                  \
        __typeof__(ret) errnum = (ret);                                 \
        if (__builtin_expect(errnum != 0, 0))                           \
            __assert_perror_fail(errnum, __FILE__, __LINE__, __func__); \
    })
#else  // CHECK_PTHREAD_RETURN_VALUE
#define MCHECK(ret)                     \
    ({                                  \
        __typeof__(ret) errnum = (ret); \
        assert(errnum == 0);            \
        (void)errnum;                   \
    })
#endif  // CHECK_PTHREAD_RETURN_VALUE

namespace Lux {
// Use as data member of a class, e.g.
//
// class Foo
// {
//  public:
//   int size() const;
//
//  private:
//   mutable MutexLock mutex_;
//   std::vector<int> data_ GUARDED_BY(mutex_);
// };

/**
 * phtread 中 mutex 核心函数：创建、销毁、加锁、解锁
 * pthread_mutex_init
 * pthread_mutex_destroy
 * pthread_mutex_lock
 * pthread_mutex_unlock
 */
class CAPABILITY("mutex") MutexLock {
    MutexLock(const MutexLock&) = delete;
    MutexLock& operator=(MutexLock&) = delete;

private:
    inline void unassignHolder() { holder_ = 0; }
    /// @brief 持有该互斥量的线程 ID(tid)
    inline void assignHolder() { holder_ = CurrentThread::tid(); }

    /// @brief 友员类 - 条件变量
    friend class Condition;

    class UnassignGuard {
        UnassignGuard(const UnassignGuard&) = delete;
        UnassignGuard& operator=(UnassignGuard&) = delete;

    public:
        explicit UnassignGuard(MutexLock& owner) : owner_(owner) {
            owner_.unassignHolder();
        }

        ~UnassignGuard() { owner_.assignHolder(); }

    private:
        MutexLock& owner_;
    };

    /// @brief  mutex attr
    pthread_mutexattr_t mutexAttr_;
    /// @brief 互斥量
    pthread_mutex_t mutex_;
    /// @brief 持有该互斥量的线程ID pid_t
    pid_t holder_;

public:
    /// @brief 互斥量初始化为默认属性
    MutexLock() : holder_(0) {
        MCHECK(pthread_mutexattr_init(&mutexAttr_));
        MCHECK(pthread_mutexattr_settype(&mutexAttr_, PTHREAD_MUTEX_NORMAL));
        MCHECK(pthread_mutex_init(&mutex_, &mutexAttr_));
    }

    ~MutexLock() {
        assert(holder_ == 0);
        MCHECK(pthread_mutex_destroy(&mutex_));
    }

    /// @brief must be called when locked, i.e. for assertion
    /// @return holder_ == CurrentThread::tid()
    inline bool isLockedByThisThread() const {
        return holder_ == CurrentThread::tid();
    }

    inline void assertLocked() const ASSERT_CAPABILITY(this) {
        assert(isLockedByThisThread());
    }

    /**
     * @brief internal usage
     * 仅供 MutexLockGuard 调用
     * 严禁用户代码调用
     */
    inline void lock() ACQUIRE() {
        // 顺序不能反
        MCHECK(pthread_mutex_lock(&mutex_));
        assignHolder();
    }

    /**
     * @brief internal usage
     * 仅供 MutexLockGuard 调用
     * 严禁用户代码调用
     */
    inline void unlock() RELEASE() {
        unassignHolder();
        MCHECK(pthread_mutex_unlock(&mutex_));
    }

    /// @brief get non-const mutex pointer
    /// 仅供 Condition 调用 严禁用户代码调用
    /// @return pthread_mutex_t*
    inline pthread_mutex_t* getPthreadMutex() /* non-const */ {
        return &mutex_;
    }
};

// Use as a stack variable, eg.
// int Foo::size() const
// {
//   MutexLockGuard lock(mutex_);
//   return data_.size();
// }
class SCOPED_CAPABILITY MutexLockGuard {
    MutexLockGuard(const MutexLockGuard&) = delete;
    MutexLockGuard& operator=(MutexLockGuard&) = delete;

private:
    MutexLock& mutex_;

public:
    explicit MutexLockGuard(MutexLock& mutex) ACQUIRE(mutex) : mutex_(mutex) {
        mutex_.lock();
    }

    ~MutexLockGuard() RELEASE() { mutex_.unlock(); }
};
}  // namespace Lux

/**
 * @brief 防止像下面这样的误用：MutexLockGuard(mutex_)
 * 临时对象不会长时间持有锁，产生的临时对象马上被销毁了，没有锁住临界区
 */
#define MutexLockGuard(x) error "Missing guard object name"
