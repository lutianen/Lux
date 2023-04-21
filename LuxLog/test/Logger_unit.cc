#include <LuxLog/AsyncLogger.h>
#include <LuxLog/Logger.h>
#include <LuxUtils/Timestamp.h>
#include <sys/resource.h>
#include <unistd.h>

#include <iostream>

off_t kRollSize = 500 * 1000 * 1000;
Lux::AsyncLogger* g_asyncLog = nullptr;

void asyncOutput(const char* msg, int len) { g_asyncLog->append(msg, len); }

void test() { LOG_TRACE << "Here is test"; }

void bench(bool longLog) {
    Lux::Logger::setOutput(asyncOutput);

    int cnt = 0;
    const int kBatch = 1000000;
    std::string empty = " ";
    std::string longStr(3000, 'X');
    longStr += " ";

    for (int t = 0; t < 10; ++t) {
        Lux::Timestamp start = Lux::Timestamp().now();
        for (int i = 0; i < kBatch; ++i) {
            LOG_INFO << "Hello 0123456789"
                     << " abcdefghijklmnopqrstuvwxyz "
                     << (longLog ? longStr : empty) << cnt;
            ++cnt;
        }
        Lux::Timestamp end = Lux::Timestamp().now();

        printf("%d: %fms\n", t,
               timeDifference(end, start) *
                   Lux::Timestamp::kMicroSecondsPerSecond / kBatch);
        struct timespec ts = {0, 500 * 1000 * 1000};
        nanosleep(&ts, NULL);
    }
}

int main(int argc, char** argv) {
    {
        // set max virtual memory to 2GB.
        size_t kOneGB = 1000 * 1024 * 1024;
        rlimit rl = {2 * kOneGB, 2 * kOneGB};
        setrlimit(RLIMIT_AS, &rl);
    }

    std::cout << "pid: " << getpid() << std::endl;

    char name[256] = {"test.log\0"};
    // strncpy(name, argv[0], sizeof(name) - 1);

    Lux::Logger::setLogLevel(Lux::Logger::LogLevel::TRACE);

    Lux::AsyncLogger log(::basename(name), kRollSize);
    log.start();
    g_asyncLog = &log;

    bool longLog = argc > 1;
    bench(longLog);

    LOG_TRACE << "This is log TRACE TEST";
    LOG_DEBUG << "This is log DEBUG TEST";
    LOG_INFO << "This is log INFO TEST";
    LOG_WARN << "This is log WARN TEST";
    LOG_ERROR << "This is log ERROR TEST";
    LOG_SYSERR << "This is log SYSERR TEST";

    test();
}
