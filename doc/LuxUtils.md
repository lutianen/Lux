# LuxUtils 子项目

常用函数库，包括有 `Timestamp`、`Mutex`、 `Condition`、 `Thread`、 `ProcessInfo`、`INI`、`Exception` 等.


## Core Class

### Timestamp

- 继承自`copyable`类，即 **默认构造、析构函数** 为编译器默认提供版，可以拷贝

- `Timestamp`类维护的私有成员：

  - 精度：毫秒级

  ```c ++
  /// @brief the number of micro seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC). (微秒)
  int64_t microSecondsSinceEpoch_;
  ```

- 核心函数：

  ```c++
  /**
   * @brief Get the Timestamp since the Epoch, 1970-01-01 00:00:00 +0000 (UTC).
   * Must use the `toString` or `toFormatStrint` to format
   * @return Timestamp 
   */
  Timestamp
  Timestamp::now() {
      struct timeval tv;
      ::gettimeofday(&tv, NULL);
      int64_t seconds = tv.tv_sec + DELTA_SECONDS_FROM_LOCAL_TIMEZONE_TO_ZORO;    
      return Timestamp(seconds * 1000 * 1000 + tv.tv_usec);
  }
  ```

- 维护`swap(Timestamp& that) { std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_); }`函数，属于不抛出异常类型函数，对应于《Effective C++》中的条款25：考虑写出一个不抛异常的 swap 函数。

  - `swap`（置换）两对象值，即将两对象的值彼此赋予对方

    ```c++
    namespace std {
        /* std::swap 的典型实现 */
        template <typename T>
        void swap(T& a, T& b) {
            T temp(a); // copy constructor
            a = b; // copy assignment
            b = temp; // copy assignment
        }
    }
    ```

  - **只要类型 T 支持 *copying* （通过 *copy* 构造函数和 *copy assignment* 操作符完成），缺省的`swap`实现代码就会帮你置换类型为 T 的对象，你不需要为此另外再作任何工作**

  - **以指针指向一个对象，内含真正数据**，这种设计的常见表现形式是所谓的 **piml 手法**（pointer to implementation）

    这时 swap 只需要交换两根指针即可，但缺省的 swap 算法不知道这一点：它不仅复制对象，而且还复制对象内维护的指针，缺乏效率。

    解决方案：提供一个特化版，以供使用。

---


### Mutex

mutex 互斥量，本质上是一把锁。

- 在访问共享资源前对互斥量进行设置（加锁），访问完成后释放（解锁）互斥量。
- 互斥量使用 pthread_mutex_t 数据类型表示
- 调用 pthread_mutex_init 函数进行初始化
- 对互斥量加锁调用 int pthread_mutex_lock(pthread_mutex_t *mutex)
- 对互斥量解锁锁调用 int pthread_mutex_unlock(pthread_mutex_t *mutex)

---

### Atomic

原子类型是封装了一个值的类型.

- 它的访问保证不会导致数据的竞争，并且可以用于在不同的线程之间同步内存访问.

GCC 4.1.2版本之后，对X86或X86_64支持内置原子操作。

- 就是说，不需要引入第三方库（如pthread）的锁保护，即可对1、2、4、8字节的数值或指针类型，进行原子加/减/与/或/异或等操作。

- 根据GCC手册中《Using the GNU Compiler Collection (GCC)》章节内容，将__sync_系列17个函数声明整理简化如下：

  - 将 `value` 和 `*ptr` 进行 加/减/或/与/异或/取反，结果更新到`*ptr`，并返回操作之前`*ptr`的值

    `type __sync_fetch_and_add (type *ptr, type value, ...)`

    `type __sync_fetch_and_sub (type *ptr, type value, ...)`

    `type __sync_fetch_and_or (type *ptr, type value, ...)`

    `type __sync_fetch_and_and (type *ptr, type value, ...)`

    `type __sync_fetch_and_xor (type *ptr, type value, ...)`

    `type __sync_fetch_and_nand (type *ptr, type value, ...)`

  - 将 `value` 和 `*ptr` 进行 加/减/或/与/异或/取反，结果更新到`*ptr`，并返回操作之后`*ptr`的值

    `type __sync_add_and_fetch (type *ptr, type value, ...)`

    `type __sync_sub_and_fetch (type *ptr, type value, ...)`

    `type __sync_or_and_fetch (type *ptr, type value, ...)`

    `type __sync_and_and_fetch (type *ptr, type value, ...)`

    `type __sync_xor_and_fetch (type *ptr, type value, ...)`

    `type __sync_nand_and_fetch (type *ptr, type value, ...)`

  - 比较 `*ptr` 与 `oldval` 的值，如果两者相等，则将`newval`更新到`*ptr`并返回`true`

    `bool __sync_bool_compare_and_swap (type *ptr, type oldval type newval, ...)`

  - 比较`*ptr`与`oldval`的值，如果两者相等，则将`newval`更新到`*ptr`并返回操作之前`*ptr`的值

    `ype __sync_val_compare_and_swap (type *ptr, type oldval type newval, ...)`

  - 发出完整内存栅栏

    `__sync_synchronize (...)`

  - 将`value`写入`*ptr`，对`*ptr`加锁，并返回操作之前`*ptr`的值。即，try spinlock 语义

    `type __sync_lock_test_and_set (type *ptr, type value, ...)`

  - 将`0`写入到`*ptr`，并对`*ptr`解锁。即，unlock spinlock语义

    `void __sync_lock_release (type *ptr, ...)`

- NOTE：

  - 这个**type**不能乱用(type只能是int, long, long long以及对应的unsigned类型)，同时在用gcc编译的时候要加上选项 -march=i686。

  - 后面的可扩展参数(…)用来指出哪些变量需要memory barrier，因为目前gcc实现的是full barrier(类似Linux kernel中的mb()，表示这个操作之前的所有内存操作不会被重排到这个操作之后)，所以可以忽略掉这个参数。

---

### Condition

Cond(条件变量)，与互斥量一起使用，允许线程以无竞争的方式等待特定的条件发生.

- 条件本身是由互斥量保护的
- 线程在改变条件状态之前必须首先锁住互斥量
- 条件变量使用 `pthread_cond_t` 数据类型表示
- 把 `PTHREAD_COND_INITIALIZER` 赋值给静态分配的条件变量进行初始化
- 使用 `int pthread_cond_init(pthread_cond_t *cond,  const pthread_condattr_t *restrict attr)` 对动态分配的条件变量进行初始化
- 使用 `int pthread_cond_destroy(pthread_cond_t *cond)` 进行反初始化
- 使用 `int pthread_cond_wait` 等待条件变量变为真，如果在给定的时间内条件不能满足，则会生成一个返回错误码的变量
- `pthread_cond_signal` 至少唤醒一个等待该条件的线程
- `pthread_cond_broadcast` 唤醒等待该条件的所有线程

---

### CountDownLatch

- pthread_join:

  是只要线程 active 就会阻塞，线程结束就会返回，一般用于主线程回收工作线程

- CountDownLatch:
  - 可以保证工作线程的任务执行完毕，主线程再对工作线程进行回收
  - 本质上来说,是一个thread safe的计数器,用于主线程和工作线程的同步
  - 用法：
    - 在初始化时,需要指定主线程需要等待的任务的个数(count), 当工作线程完成 Task Callback后对计数器减1，而主线程通过wait()调用阻塞等待技术器减到0为止.
    - 初始化计数器值为1,在程序结尾将创建一个线程执行CountDown操作并wait()，当程序执行到最后会阻塞直到计数器减为0,这可以保证线程池中的线程都start了线程池对象才完成析构,实现ThreadPool的过程中遇到过

  - 核心函数：
    - `countDwon()`

      对计数器进行原子减一操作

    - `wait()`

      使用条件变量等待计数器减到零，然后notify

---