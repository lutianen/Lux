/**
 * @file LogFile.h
 * @brief
 *
 * @author Tianen Lu
 */

#pragma once

#include <LuxUtils/Mutex.h>
#include <LuxUtils/Types.h>

#include <memory>

namespace Lux {
namespace FileUtil {
class AppendFile;
}  // namespace FileUtil

class LogFile {
    LogFile(const LogFile&) = delete;
    LogFile& operator=(LogFile&) = delete;

private:
    void append_unlocked(const char* logline, int len);

    static string getLogFileName(const string& basename, time_t* now);

    const string basename_;
    const off_t rollSize_;
    const int flushInterval_;
    const int checkEveryN_;

    int count_;

    std::unique_ptr<MutexLock> mutex_;
    time_t startOfPeriod_;
    time_t lastRoll_;
    time_t lastFlush_;
    std::unique_ptr<FileUtil::AppendFile> file_;

    const static int kRollPerSeconds_ = 60 * 60 * 24;

public:
    LogFile(const string& basename, off_t rollSize, bool threadSafe = true,
            int flushInterval = 3, int checkEveryN = 1024);
    ~LogFile();

    void append(const char* logline, int len);
    void flush();
    bool rollFile();
};

}  // namespace Lux