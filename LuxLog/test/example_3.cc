// Required headers
#include <LuxLog/AsyncLogger.h>
#include <LuxLog/Logger.h>
#include <sys/resource.h>
#include <unistd.h>

#include <LuxUtils/LuxINI.hpp>
#include <cstdio>

// Global logger.
Lux::AsyncLogger* g_asyncLog = nullptr;

int main(int argc, char** argv) {
    {
        // Optional: set max virtual memory to 2GB.
        size_t kOneGB = 1000 * 1024 * 1024;

        rlimit rl = {2 * kOneGB, 2 * kOneGB};
        setrlimit(RLIMIT_AS, &rl);
    }

    // Read ini
    Lux::INIParser::INI iniF("/home/lutianen/Lux/LuxLog/test/logger.ini");
    Lux::INIParser::INIStructure iniC;
    iniF.read(iniC);

    // Configure logger
    Lux::string name = iniC["LuxLogger"]["basename"];
    off_t rollSize = atoi(iniC["LuxLogger"]["rollSize"].c_str());
    int flushInterval = atoi(iniC["LuxLogger"]["flushInterval"].c_str());

    Lux::AsyncLogger log(::basename(name.c_str()), rollSize, flushInterval);

    // Set level of logger
    Lux::Logger::setLogLevel(Lux::Logger::LogLevel::TRACE);

    // set output Func, or use defalut.
    Lux::Logger::setOutput([](const char* msg, int len) {
        size_t n = ::fwrite(msg, 1, static_cast<size_t>(len), stdout);
        assert(n == static_cast<size_t>(len));
        if (n != static_cast<size_t>(len)) {
            char _buf[128] = {};
            ::snprintf(_buf, sizeof(_buf), "%s : %d - n should be equal len!!!",
                       __FILE__, __LINE__);
            ::perror(_buf);
        }

        g_asyncLog->append(msg, len);
    });
    g_asyncLog = &log;
    log.start();

    for (int i = 0; i < 10; ++i) {
        LOG_TRACE << "this is log trace test"
                  << " << " << i << " >>";
        LOG_DEBUG << "this is log debug test"
                  << " << " << i << " >>";
        LOG_INFO << "this is log info test"
                 << " << " << i << " >>";
        LOG_WARN << "this is log warn test"
                 << " << " << i << " >>";
        LOG_ERROR << "this is log error test"
                  << " << " << i << " >>";
        LOG_SYSERR << "this is log syserr test"
                   << " << " << i << " >>";
    }

    /* Wait backend thread write logs into file.
     * Only needed when testing LuxLogger.
     */
    usleep(5000);
}
