/**
 * @file Atomic.h
 * @brief 原子类型是封装了一个值的类型.
 *  它的访问保证不会导致数据的竞争，并且可以用于在不同的线程之间同步内存访问.
 *
 * @author Tianen Lu (tianenlu957@gmail.com)
 */

#pragma once

#include <cstdint>  // int32_t int64_t

/**
 * @brief GCC 4.1.2版本之后，对X86或X86_64支持内置原子操作。
 * 就是说，不需要引入第三方库（如pthread）的锁保护，
 * 即可对1、2、4、8字节的数值或指针类型，进行原子加/减/与/或/异或等操作。
 *
 * 根据GCC手册中《Using the GNU Compiler Collection (GCC)》章节内容，
 * 将__sync_系列17个函数声明整理简化如下：
 *      将 value 和 *ptr 进行
 * 加/减/或/与/异或/取反，结果更新到*ptr，并返回操作之前*ptr的值
 *  - type __sync_fetch_and_add (type *ptr, type value, ...)
 *  - type __sync_fetch_and_sub (type *ptr, type value, ...)
 *  - type __sync_fetch_and_or (type *ptr, type value, ...)
 *  - type __sync_fetch_and_and (type *ptr, type value, ...)
 *  - type __sync_fetch_and_xor (type *ptr, type value, ...)
 *  - type __sync_fetch_and_nand (type *ptr, type value, ...)
 *
 *      将 value 和 *ptr 进行
 * 加/减/或/与/异或/取反，结果更新到*ptr，并返回操作之后*ptr的值
 *  - type __sync_add_and_fetch (type *ptr, type value, ...)
 *  - type __sync_sub_and_fetch (type *ptr, type value, ...)
 *  - type __sync_or_and_fetch (type *ptr, type value, ...)
 *  - type __sync_and_and_fetch (type *ptr, type value, ...)
 *  - type __sync_xor_and_fetch (type *ptr, type value, ...)
 *  - type __sync_nand_and_fetch (type *ptr, type value, ...)
 *
 *      比较 *ptr 与 oldval 的值，如果两者相等，则将newval更新到*ptr并返回true
 *  - bool __sync_bool_compare_and_swap (type *ptr, type oldval type newval,
 * ...)
 *
 *
 比较*ptr与oldval的值，如果两者相等，则将newval更新到*ptr并返回操作之前*ptr的值
 *  - type __sync_val_compare_and_swap (type *ptr, type oldval type newval, ...)
 *
 *      发出完整内存栅栏
 *  - __sync_synchronize (...)
 *
 *      将value写入*ptr，对*ptr加锁，并返回操作之前*ptr的值。即，try
 * spinlock语义
 *  - type __sync_lock_test_and_set (type *ptr, type value, ...)
 *
 *      将0写入到*ptr，并对*ptr解锁。即，unlock spinlock语义
 *  - void __sync_lock_release (type *ptr, ...)
 *
 * 这个type不能乱用(type只能是int, long, long long以及对应的unsigned类型)，
 * 同时在用gcc编译的时候要加上选项 -march=i686
 *
 * 后面的可扩展参数(…)用来指出哪些变量需要memory barrier，
 * 因为目前gcc实现的是full barrier(类似Linuxkernel中的mb()，
 * 表示这个操作之前的所有内存操作不会被重排到这个操作之后)，所以可以忽略掉这个参数。
 */

namespace Lux {
namespace detail {
template <typename T>
class AtomicIntegerT {
private:
    volatile T value_;

    AtomicIntegerT(const AtomicIntegerT&) = delete;
    AtomicIntegerT& operator=(AtomicIntegerT&) = delete;

public:
    AtomicIntegerT() : value_(0) {}

    // uncomment if you need copying and assignment
    //
    // AtomicIntegerT(const AtomicIntegerT& that)
    //   : value_(that.get())
    // {}
    //
    // AtomicIntegerT& operator=(const AtomicIntegerT& that)
    // {
    //   getAndSet(that.get());
    //   return *this;
    // }

    /// @brief CAS compare and swap
    /// @return
    T get() {
        // in gcc >= 4.7: __atomic_load_n(&value_, __ATOMIC_SEQ_CST)
        return __sync_val_compare_and_swap(&value_, 0, 0);
    }

    T getAndAdd(T x) {
        // in gcc >= 4.7: __atomic_fetch_add(&value_, x,
        // __ATOMIC_SEQ_CST)
        return __sync_fetch_and_add(&value_, x);
    }

    T addAndGet(T x) { return getAndAdd(x) + x; }

    T incrementAndGet() { return addAndGet(1); }

    T decrementAndGet() { return addAndGet(-1); }

    void add(T x) { getAndAdd(x); }

    void increment() { incrementAndGet(); }

    void decrement() { decrementAndGet(); }

    T getAndSet(T newValue) {
        // in gcc >= 4.7: __atomic_exchange_n(&value_, newValue,
        // __ATOMIC_SEQ_CST)
        return __sync_lock_test_and_set(&value_, newValue);
    }
};
}  // namespace detail

typedef detail::AtomicIntegerT<int32_t> AtomicInt32;
typedef detail::AtomicIntegerT<int64_t> AtomicInt64;

}  // namespace Lux