/**
 * @file AsyncLogger.cc
 * @brief AsyncLogger
 *  - 异步日志系统，采用多线程模拟异步IO，采用双缓冲(Double
 * Buffering)机制，运行时日志输出级别可调
 *  - 多生产者-单消费者问题：
 *      - 生产者（前端），要尽量做到低延迟、低CPU开销、无阻塞
 *      - 消费者（后端），要做到足够大的吞吐量，并占用较少资源
 *  - Double Buffer:
 *      基本思路：
 *          0. 准备两块 Buffer A B
 *          1. 前端负责往 Buffer A 中填数据（日志信息）
 *          2. 后端负责把 Buffer B 中数据写入文件
 *          ×× 当 Buffer A 写满后，交换 A 和 B，让后端将 Buffer A
 * 中的数据写入文件，而前端 则往 Buffer B 填入新的日志信息，如此反复 ×× 优点：
 *          双缓冲机制为什么高效？
 *          -
 * 在大部分的时间中，前台线程和后台线程不会操作同一个缓冲区，这也就意味着前台线程的操作，
 *            不需要等待后台线程缓慢的写文件操作(因为不需要锁定临界区)。
 *          -
 * 后台线程把缓冲区中的日志信息，写入到文件系统中的频率，完全由自己的写入策略来决定，避
 *            免了每条新日志信息都触发(唤醒)后端日志线程
 *              例如，可以根据实际场景，定义一个刷新频率（2秒），只要刷新时间到了，即使缓冲区中的
 *              文件很少，也要把他们存储到文件系统中
 *          -
 * 换言之，前端线程不是将一条条日志信息分别传送给后端线程，而是将多条信息拼成一个大的
 * buffer 传送给后端，相当于是批量处理，减少了线程唤醒的频率，降低开销
 *
 *          如何做到尽可能降低 lock 的时间？
 *          -
 * 在[大部分的时间中]，前台线程和后台线程不会操作同一个缓冲区。也就是是说，在小部分时间内，
 *            它们还是有可能操作同一个缓冲区的。就是：当前台的写入缓冲区 buffer
 * A 被写满了，需要 与 buffer B
 * 进行交换的时候，交换的操作，是由后台线程来执行的，具体流程是：
 *              - 后台线程被唤醒，此时 buffer B
 * 缓冲区是空的，因为在上一次进入睡眠之前，buffer B
 *                中数据已经被写入到文件系统中了
 *              - 把 buffer A 与 buffer B 进行交换
 *              - 把 buffer B 中的数据写入到文件系统
 *              - 开始休眠
 *          -
 * 交换缓冲区，就是把两个指针变量的值交换一下而已，利用C++语言中的swap操作，效率很高
 *          -
 * 在执行交换缓冲区的时候，可能会有前台线程写入日志，因此这个步骤需要在 Lock
 * 的状态下执行
 *          - ××
 * 这个双缓冲机制的前后台日志系统，需要锁定的代码仅仅是交换两个缓冲区这个动作，
 *               Lock 的时间是极其短暂的！这就是它提高吞吐量的关键所在！ ××
 *
 *          C++ stream 风格：
 *          - 用起来更自然，不必费心保持格式字符串与参数类型的一致性
 *          - 可以随用随写
 *          - 类型安全
 *          -
 * 当输出的日志级别高于语句的日志级别时，打印日志是个空操作，运行时开销接近零
 *
 *          日志文件滚动 rolling：
 *          -
 * 条件：文件大小、时间（例如，每天零点新建一个日志文件，不论前一个日志文件是否写满）
 *          - 自动根据文件大小和时间来主动滚动日志文件
 *          - 不采用文件改名的方法
 *
 *          日志文件名：
 *          - xxx.20220912-012345.hostname.2333.log
 *          -
 * 第一部分：xxx，进程的名字，容易区分是哪个服务程序的日志，[必要时，可加入版本号]
 *          -
 * 第二部分：.20220912-012345，文件的创建时间(精确到秒)，可利用通配符`*.20220912-01*`筛选日志
 *          - 第三部分：hostname，机器名称，便于溯源
 *          -
 * 第四部分：2333,进程ID，如果一个程序一秒之内反复重启，那么每次都会生成不同的日志
 *          - 第五部分：统一的后缀名.log，便于配套脚本的编写
 *  -
 * 实现输出操作的是一个线程，那么在写入期间，这个线程就需要一直持有缓冲区中的日志数据。
 *
 *  - 可以继续优化的地方：
 *    异步日志系统中，使用了一个全局锁，尽管临界区很小，但是如果线程数目较多，锁争用也可能影响性能。
 *    一种解决方法是像 Java 的 ConCurrentHashMap
 * 那样使用多个桶子(bucket)，前端线程写日志的时候 根据线程id哈希到不同的 bucket
 * 中，以减少竞争。
 *    这种解决方案本质上就是提供更多的缓冲区，并且把不同的缓冲区分配给不同的线程(根据线程
 * id 的哈希值)。
 * 那些哈希到相同缓冲区的线程，同样是存在争用的情况的，只不过争用的概率被降低了很多。
 *
 * @author Tianen Lu (tianenlu957@gmail.com)
 */

#include <LuxLog/AsyncLogger.h>
#include <LuxLog/LogFile.h>
#include <LuxUtils/Timestamp.h>

using namespace Lux;

/// @brief Constructor
/// @param basename
/// @param rollSize
/// @param flushInterval
AsyncLogger::AsyncLogger(const string& basename, off_t rollSize,
                         int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(basename),
      rollSize_(rollSize),
      thread_(std::bind(&AsyncLogger::threadFunc, this), "AsyncLogger"),
      latch_(1),
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_() {
    currentBuffer_->bzero();
    nextBuffer_->bzero();

    // vector 的reserve增加了vector的capacity，但是它的size没有改变
    // 而resize改变了vector的capacity同时也增加了它的size
    buffers_.reserve(16);
}

/// @brief 前端线程调用，把日志信息放入缓冲
/// @param logline
/// @param len
void AsyncLogger::append(const char* logline, int len) {
    MutexLockGuard lock(mutex_);

    /// 当前写缓冲有足够的空间放置日志信息
    /// 直接放入
    if (currentBuffer_->avail() > len) {
        currentBuffer_->append(logline, static_cast<size_t>(len));
    } else {  /// 当前缓冲空间不足
        /// 将当前缓冲移动到 buffers_ 集合中，等待写入文件系统
        buffers_.push_back(std::move(currentBuffer_));  /// 利用 移动 而非 复制

        /// 如果 预备缓冲 未被移动，则将预备缓冲移动做到当前缓冲
        /// 也就是说，前端线程的写入速度小于后端线程的文件写入速度
        if (nextBuffer_) {
            currentBuffer_ = std::move(nextBuffer_);
        } else {  /// 前端线程写入太快，需要重新申请一块新的缓冲作为当前缓冲
            // Rarely happens
            currentBuffer_.reset(new Buffer);
        }

        /// 日志文件写入
        currentBuffer_->append(logline, static_cast<size_t>(len));
        cond_.notify();
    }
}

/// @brief 后端线程调用，把日志信息写入文件系统
void AsyncLogger::threadFunc() {
    assert(running_ == true);
    latch_.countDown();

    // LogFile output(basename_, rollSize_, false);
    LogFile output(basename_, rollSize_, false, flushInterval_);

    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();

    /// 申请一块大小为16的缓冲集，一般只会用到2块，除非前端写入速度太快
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);

    while (running_) {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        /// Swap out what need to be written, keep CS short
        {
            Lux::MutexLockGuard lock(mutex_);
            if (buffers_.empty())  // unusual usage!
                cond_.waitForSeconds(flushInterval_);

            /// 采用move 提高效率
            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer1);
            /// 内部指针交换 而非复制
            /// 最核心操作，前后端缓冲交换
            buffersToWrite.swap(buffers_);
            if (!nextBuffer_) nextBuffer_ = std::move(newBuffer2);
        }

        assert(!buffersToWrite.empty());

        /// 待写入缓冲集长度不对 输出错误
        /// 将错误数据写入文件，并裁剪待写入缓冲集
        if (buffersToWrite.size() > 25) {
            char buf[256];
            snprintf(buf, sizeof buf,
                     "Dropped log messages at %s, %zd larger buffers\n",
                     Timestamp::now().toFormattedString().c_str(),
                     buffersToWrite.size() - 2);
            fputs(buf, stderr);
            output.append(buf, static_cast<int>(strlen(buf)));
            buffersToWrite.erase(buffersToWrite.begin() + 2,
                                 buffersToWrite.end());
        }

        /// 迭代待写入缓冲集，将缓冲日志写入文件系统
        for (const auto& buffer : buffersToWrite) {
            // FIXME: use unbuffered stdio FILE ? or use ::writev ?
            output.append(buffer->data(), buffer->length());
        }

        if (buffersToWrite.size() > 2) {
            // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);
        }

        if (!newBuffer1) {
            assert(!buffersToWrite.empty());
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2) {
            assert(!buffersToWrite.empty());
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }

    output.flush();
}
