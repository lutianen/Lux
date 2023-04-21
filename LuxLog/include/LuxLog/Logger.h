/**
 * @file Logger.h
 * @brief
 *
 * @author Tianen Lu (tianenlu957@gmail.com)
 */

#pragma once

#include <LuxLog/LogStream.h>
#include <LuxUtils/Timestamp.h>

namespace Lux {
class Logger {
public:
    enum class LogLevel {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS,
    };

    /// @brief compile time calculation of basename of source file
    class SourceFile {
    public:
        template <int N>
        SourceFile(const char (&arr)[N]) : data_(arr), size_(N - 1) {
            // builtin function
            const char* slash = ::strrchr(data_, '/');
            if (slash) {
                data_ = slash + 1;
                size_ -= static_cast<int>(data_ - arr);
            }
        }

        explicit SourceFile(const char* filename) : data_(filename) {
            const char* slash = ::strrchr(filename, '/');
            if (slash) {
                data_ = slash + 1;
            }
            size_ = static_cast<int>(::strlen(data_));
        }

        const char* data_;
        int size_;
    };

private:
    class Impl {
    public:
        using LogLevel = Logger::LogLevel;

        /// @brief Constructor
        Impl(LogLevel level, int old_errno, const SourceFile& file, int line);

        /**
         * @brief 将本地时间格式化
         */
        void formatTime();
        void finish();

        Timestamp time_;
        LogStream stream_;
        LogLevel level_;
        int line_;
        SourceFile basename_;
    };

    Impl impl_;

public:
    /// @brief Constructor and Destructor
    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, LogLevel level, const char* func);
    Logger(SourceFile file, int line, bool toAbort);
    ~Logger();

    LogStream& stream() { return impl_.stream_; }

    static LogLevel logLevel();
    static void setLogLevel(LogLevel level);

    typedef void (*OutputFunc)(const char* msg, int len);
    typedef void (*FlushFunc)();
    static void setOutput(OutputFunc);
    static void setFlush(FlushFunc);
};

/* NOTE Global logger level */
extern Logger::LogLevel g_logLevel;
inline Logger::LogLevel Logger::logLevel() { return g_logLevel; }

//
// CAUTION: do not write:
//
// if (good)
//   LOG_INFO << "Good news";
// else
//   LOG_WARN << "Bad news";
//
// this expends to
//
// if (good)
//   if (logging_INFO)
//     logInfoStream << "Good news";
//   else
//     logWarnStream << "Bad news";
//
#define LOG_TRACE                                                           \
    if (Lux::Logger::logLevel() <= Lux::Logger::LogLevel::TRACE)            \
    Lux::Logger(__FILE__, __LINE__, Lux::Logger::LogLevel::TRACE, __func__) \
        .stream()
#define LOG_DEBUG                                                           \
    if (Lux::Logger::logLevel() <= Lux::Logger::LogLevel::DEBUG)            \
    Lux::Logger(__FILE__, __LINE__, Lux::Logger::LogLevel::DEBUG, __func__) \
        .stream()
#define LOG_INFO                                                \
    if (Lux::Logger::logLevel() <= Lux::Logger::LogLevel::INFO) \
    Lux::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN \
    Lux::Logger(__FILE__, __LINE__, Lux::Logger::LogLevel::WARN).stream()
#define LOG_ERROR \
    Lux::Logger(__FILE__, __LINE__, Lux::Logger::LogLevel::ERROR).stream()
#define LOG_FATAL \
    Lux::Logger(__FILE__, __LINE__, Lux::Logger::LogLevel::FATAL).stream()
#define LOG_SYSERR Lux::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL Lux::Logger(__FILE__, __LINE__, true).stream()

const char* strerror_tl(int savedErrno);

// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.
#define CHECK_NOTNULL(val)                                                 \
    ::Lux::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", \
                        (val))

// A small helper for CHECK_NOTNULL().
template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char* names, T* ptr) {
    if (ptr == NULL) {
        Logger(file, line, Logger::LogLevel::FATAL).stream() << names;
    }
    return ptr;
}
}  // namespace Lux
