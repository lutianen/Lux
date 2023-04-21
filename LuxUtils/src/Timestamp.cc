/**
 * @file Timestamp.cc
 * @brief
 *
 * @author Tianen Lu (tianenlu957@gmail.com)
 */

#include <LuxUtils/Timestamp.h>
#include <inttypes.h>  // PRId64
#include <sys/time.h>  // gettimeofday

#include <ctime>  // tm

// FIXME: Modify accoding to Local Timezone
static const int64_t DELTA_SECONDS_FROM_LOCAL_TIMEZONE_TO_ZORO = 8 * 60 * 60;

using namespace Lux;

/* Timestamp 的内存分布大小 sizeof(int64_t) */
static_assert(sizeof(Timestamp) == sizeof(int64_t),
              "Timestamp should be same size as int64_t");

/**
 * @brief Get the Timestamp since the Epoch, 1970-01-01 00:00:00 +0000 (UTC).
 * Must use the `toString` or `toFormatStrint` to format
 * @return Timestamp
 */
Timestamp Timestamp::now() {
    struct timeval tv;
    ::gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec + DELTA_SECONDS_FROM_LOCAL_TIMEZONE_TO_ZORO;
    return Timestamp(seconds * 1000 * 1000 + tv.tv_usec);
}

/**
 * @brief toString -
 * 将 Timestamp 中 microSecondsSinceEpoch_ 转换成字符串
 * @return string
 */
string Timestamp::toString() const {
    char buf[32] = {0};
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
    snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64 "", seconds,
             microseconds);
    return buf;
}

/**
 * @brief toFormattedString -
 * 显示本地的当前时间 - YYYY/MM/DD HH:mm:ss.ffffff
 * 默认不显示 毫秒
 * @return string
 */
string Timestamp::toFormattedString(bool showMicroSecond) const {
    char buf[64] = {0};
    // tm* tm_time = localtime(&microSecondsSinceEpoch_);
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);

    if (showMicroSecond) {
        int microseconds =
            static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d/%02d/%02d %02d:%02d:%02d.%06d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);
    } else {
        snprintf(buf, sizeof(buf), "%4d/%02d/%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    return buf;
}
