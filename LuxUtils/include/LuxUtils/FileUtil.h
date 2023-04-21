/**
 * @file FileUtil.h
 * @brief
 *
 * @author Tianen Lu (tianenlu957@gmail.com)
 */

#pragma once

#include <LuxUtils/StringPiece.h>
#include <sys/types.h>  // off_t

namespace Lux {
namespace FileUtil {
/// @brief read small file < 64KB
class ReadSmallFile {
    ReadSmallFile(const ReadSmallFile&) = delete;
    ReadSmallFile(ReadSmallFile&) = delete;

public:
    static const int kBufferSize = 64 * 1024;

private:
    int fd_;
    int err_;
    char buf_[kBufferSize];

public:
    ReadSmallFile(StringArg filename);
    ~ReadSmallFile();

    /// @brief
    template <typename String>
    int readToString(int maxSize, String* content, int64_t* fileSize,
                     int64_t* modifyTime, int64_t* createTime);

    /// @brief Read at maxium kBufferSize into buf_
    int readToBuffer(int* size);

    const char* buffer() const { return buf_; }
};

/// @brief read the file content, returns errno if error happens.
template <typename String>
int readFile(StringArg filename, int maxSize, String* content,
             int64_t* fileSize = NULL, int64_t* modifyTime = NULL,
             int64_t* createTime = NULL) {
    ReadSmallFile file(filename);
    return file.readToString(maxSize, content, fileSize, modifyTime,
                             createTime);
}

// not thread safe
class AppendFile {
    AppendFile(const AppendFile&) = delete;
    AppendFile(AppendFile&) = delete;

private:
    size_t write(const char* logline, size_t len);

    FILE* fp_;
    char buffer_[64 * 1024];
    off_t writtenBytes_;

public:
    explicit AppendFile(StringArg filename);
    ~AppendFile();

    void append(const char* logline, size_t len);

    void flush();

    off_t writtenBytes() const { return writtenBytes_; }
};
}  // namespace FileUtil
}  // namespace Lux
