/**
 * @file Logger.cc
 * @brief
 *
 * @author Tianen Lu
 */

#include <LuxLog/Logger.h>
#include <LuxUtils/CurrentThread.h>  // tid
#include <LuxUtils/Timestamp.h>

#include <cstring>
#include <ctime>  // gmtime_r tm

namespace Lux {
/*
class LoggerImpl {
public:
    typedef Logger::LogLevel LogLevel;
    LoggerImpl(LogLevel level, int old_errno, const char* file, int line);
    void finish();

    Timestamp time_;
    LogStream stream_;
    LogLevel level_;
    int line_;
    const char* fullname_;
    const char* basename_;
};
*/

__thread char t_errnobuf[512];
__thread char t_time[64];
__thread time_t t_lastSecond;

const char* strerror_tl(int savedErrno) {
    return ::strerror_r(savedErrno, t_errnobuf, sizeof(t_errnobuf));
}

Logger::LogLevel initLogLevel() {
    if (::getenv("Lux_LOG_TRACE"))
        return Logger::LogLevel::TRACE;
    else if (::getenv("Lux_LOG_DEBUG"))
        return Logger::LogLevel::DEBUG;
    else
        return Logger::LogLevel::INFO;
}

/* NOTE Global logger level is set */
Logger::LogLevel g_logLevel = initLogLevel();

const char* LogLevelName[static_cast<unsigned int>(
    Logger::LogLevel::NUM_LOG_LEVELS)] = {
    "TRACE ", "DEBUG ", "INFO  ", "WARN  ", "ERROR ", "FATAL ",
};

// strlen
constexpr int LogLevelStrLen = 6;
constexpr char MsgDelimiter[] = ">_< ";

// helper class for known string length at compile time
class T {
public:
    T(const char* str, unsigned len) : str_(str), len_(len) {
        assert(strlen(str) == len_);
    }

    const char* str_;
    const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v) {
    s.append(v.str_, static_cast<int>(v.len_));
    return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v) {
    s.append(v.data_, v.size_);
    return s;
}

/**
 * @brief Default output Func.
 *  It push `msg` to `stdout`.
 * @param msg Message
 */
void defaultOutput(const char* msg, int len) {
    size_t n = ::fwrite(msg, 1, static_cast<size_t>(len), stdout);

    assert(n == static_cast<size_t>(len));
    if (n != static_cast<size_t>(len)) {
        char _buf[128] = {};
        ::snprintf(_buf, sizeof(_buf), "%s : %d - n should be equal len!!!",
                   __FILE__, __LINE__);
        ::perror(_buf);
    }
}

void defaultFlush() { fflush(stdout); }

/* NOTE Global Outuput/Flush Function */
Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

}  // namespace Lux

using namespace Lux;

Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file,
                   int line)
    : time_(Timestamp::now()),
      stream_(),
      level_(level),
      line_(line),
      basename_(file) {
    formatTime();
    CurrentThread::tid();
    stream_ << T(CurrentThread::tidString(),
                 static_cast<unsigned int>(CurrentThread::tidStringLength()));

    stream_ << T(LogLevelName[static_cast<unsigned int>(level)],
                 LogLevelStrLen);

    // Format Change: xxx - Channel_test.cc:104 -> Channel_test.cc:104 -
    // Channel_test.cc:104
    stream_ << basename_ << ':' << line_ << ' ';

    if (savedErrno != 0) {
        stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

/// @brief Put "YYYY/MM/DD hh:mm:ss " into stream
void Logger::Impl::formatTime() {
    int64_t secondsSinceEpoch = time_.secondsSinceEpoch();
    if (secondsSinceEpoch != t_lastSecond) {
        t_lastSecond = secondsSinceEpoch;
        struct tm tm_time {};
        ::gmtime_r(&secondsSinceEpoch, &tm_time);

        int len = snprintf(
            t_time, sizeof(t_time), "%4d/%02d/%02d %02d:%02d:%02d",
            tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 19);
        (void)len;
    }
    stream_ << T(t_time, 19);
    stream_ << T(" ", 1);
}

/**
 * @brief 调用日志的文件和行号
 * e.g. LuCCLogger_TEST.cc:7
 */
void Logger::Impl::finish() {
    // stream_ << " - " << basename_ << ':' << line_ << '\n';
    stream_ << "\n";
}

Logger::Logger(SourceFile file, int line)
    : impl_(LogLevel::INFO, 0, file, line) {
    impl_.stream_ << MsgDelimiter;
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
    : impl_(level, 0, file, line) {
    // impl_.stream_ << func << ' ';
    impl_.stream_ << func << "(..) " << MsgDelimiter;
}

Logger::Logger(SourceFile file, int line, LogLevel level)
    : impl_(level, 0, file, line) {
    impl_.stream_ << MsgDelimiter;
}

Logger::Logger(SourceFile file, int line, bool toAbort)
    : impl_(toAbort ? LogLevel::FATAL : LogLevel::ERROR, errno, file, line) {
    impl_.stream_ << MsgDelimiter;
}

Logger::~Logger() {
    impl_.finish();
    const LogStream::Buffer& buf(stream().buffer());
    g_output(buf.data(), buf.length());
    if (impl_.level_ == LogLevel::FATAL) {
        g_flush();
        abort();
    }
}

void Logger::setLogLevel(Logger::LogLevel level) { g_logLevel = level; }

void Logger::setOutput(OutputFunc out) { g_output = out; }

void Logger::setFlush(FlushFunc flush) { g_flush = flush; }
