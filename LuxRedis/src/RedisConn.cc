/**
 * @brief
 */

#include <LuxLog/Logger.h>
#include <LuxRedis/RedisConn.h>
#include <hiredis/hiredis.h>
#include <sys/types.h>

#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

#define REDIS_NORMAL_JUDGE(reply)                                  \
    if (nullptr == reply) return Lux::ErrorCode::ERedis::NetError; \
    if (REDIS_REPLY_ERROR == reply->type) {                        \
        freeReplyObject(reply);                                    \
        return Lux::ErrorCode::ERedis::NormalError;                \
    }

#ifdef __cplusplus
extern "C" {
#endif
int __redisAppendCommand(redisContext *c, const char *cmd, size_t len);
#ifdef __cplusplus
}
#endif

/* Calculate the number of bytes needed to represent an integer as string. */
static int intlen(int i) {
    int len = 0;
    if (i < 0) {
        len++;
        i = -i;
    }
    do {
        len++;
        i /= 10;
    } while (i);
    return len;
}

/* Helper that calculates the bulk length given a certain string length. */
static size_t bulklen(size_t len) { return 1 + intlen(len) + 2 + len + 2; }

using namespace Lux;
using namespace Lux::redis;

bool RedisConn::reConnectSvr() {
    disconnectSvr();
    return connectSvr(IP_, port_, timeout_);
}

/**
 * @brief Construct a new Redis Conn:: Redis Conn object
 */
RedisConn::RedisConn()
    : redisContextPtr_(nullptr), IP_("127.0.0.1"), port_(6379), timeout_(0) {}

RedisConn::~RedisConn() {}

bool RedisConn::connectSvr(const string &ip, uint16_t port,
                           unsigned int timeout) {
    IP_ = ip;
    port_ = port;
    timeout_ = timeout;

    struct timeval tvTimeout;
    tvTimeout.tv_sec = timeout / 1000;
    tvTimeout.tv_usec = timeout % 1000 * 1000;

    redisContextPtr_ = redisConnectWithTimeout(ip.c_str(), port, tvTimeout);
    if (nullptr == redisContextPtr_ || redisContextPtr_->err) {
        if (redisContextPtr_) {
            LOG_ERROR << "Redis Connection ip: " << ip << ", port: " << port
                      << ", error: " << redisContextPtr_->errstr;
            disconnectSvr();
        } else {
            LOG_ERROR
                << "redis Connection error: can't allocate redis context, ip:"
                << ip << ", port: " << port;
        }
        return false;
    }
    return true;
}

void RedisConn::disconnectSvr() {
    redisFree(redisContextPtr_);
    redisContextPtr_ = nullptr;
}

int RedisConn::asynSave() {
    redisReply *reply = reinterpret_cast<redisReply *>(command("BGSAVE"));
    REDIS_NORMAL_JUDGE(reply);

    freeReplyObject(reply);
    return 0;
}

int RedisConn::save() {
    redisReply *reply = reinterpret_cast<redisReply *>(command("SAVE"));
    REDIS_NORMAL_JUDGE(reply);

    if (0 == strncasecmp("ok", reply->str, 3)) {
        freeReplyObject(reply);
        return 0;
    }

    freeReplyObject(reply);
    return ErrorCode::ERedis::NormalError;
}

void *RedisConn::command(const char *format, ...) {
    va_list ap;
    void *reply = nullptr;
    if (redisContextPtr_ != nullptr) {
        va_start(ap, format);
        reply = redisvCommand(redisContextPtr_, format, ap);
        va_end(ap);
    }

    if (nullptr == reply) {
        LOG_INFO << "It's going to ReConnect Redis Server!";
        if (reConnectSvr()) {
            LOG_INFO << "ReConnect Server Success!";
            va_start(ap, format);
            reply = redisvCommand(redisContextPtr_, format, ap);
            va_end(ap);
        }
    }

    return reply;
}

void *RedisConn::commandArgv(const std::vector<std::string> &vstrOper,
                             const std::vector<std::string> &vstrParam) {
    char *cmd = nullptr;
    int len = 0;
    void *reply = nullptr;
    int pos;
    int totlen = 0;

    // Format command
    // Calculate number of bytes needed for the command.
    totlen = 1 + intlen(vstrOper.size() + vstrParam.size()) + 2;
    for (unsigned int j = 0; j < vstrOper.size(); ++j)
        totlen += bulklen(vstrOper[j].length());
    for (unsigned int j = 0; j < vstrParam.size(); ++j)
        totlen += bulklen(vstrParam[j].length());

    // Build the command at protocol level
    cmd = new char[totlen + 1];
    if (nullptr == cmd) return nullptr;

    pos = sprintf(cmd, "*%zu\r\n", vstrOper.size() + vstrParam.size());

    // push cmd
    for (unsigned int j = 0; j < vstrOper.size(); j++) {
        pos += sprintf(cmd + pos, "$%zu\r\n", vstrOper[j].length());
        memcpy(cmd + pos, vstrOper[j].c_str(), vstrOper[j].length());
        pos += vstrOper[j].length();
        cmd[pos++] = '\r';
        cmd[pos++] = '\n';
    }

    // push param
    for (unsigned int j = 0; j < vstrParam.size(); j++) {
        pos += sprintf(cmd + pos, "$%zu\r\n", vstrParam[j].length());
        memcpy(cmd + pos, vstrParam[j].c_str(), vstrParam[j].length());
        pos += vstrParam[j].length();
        cmd[pos++] = '\r';
        cmd[pos++] = '\n';
    }

    assert(pos == totlen);
    cmd[pos] = '\0';

    len = totlen;
    // Execute command
    reply = execCommand(cmd, len);
    if (nullptr == reply) {
        if (reConnectSvr()) reply = execCommand(cmd, len);
    }

    delete[] cmd;
    return reply;
}

void *RedisConn::execCommand(const char *cmd, int len) {
    void *reply = nullptr;
    if (nullptr == redisContextPtr_) return nullptr;

    // Execute command
    if (__redisAppendCommand(redisContextPtr_, cmd, len) != REDIS_OK)
        return nullptr;

    // Get results
    if (redisContextPtr_->flags & REDIS_BLOCK) {
        if (redisGetReply(redisContextPtr_, &reply) != REDIS_OK) return nullptr;
    }

    return reply;
}
/*****************************************************************************/

int RedisConn::setKey(const char *key, int keyLen, const char *value,
                      int valueLen, unsigned int lifeTime) {
    redisReply *reply = nullptr;
    if (0 == lifeTime)
        reply = reinterpret_cast<redisReply *>(
            command("SET %b %b", key, static_cast<size_t>(keyLen), value,
                    static_cast<size_t>(valueLen)));
    else
        reply = reinterpret_cast<redisReply *>(
            command("SET %b %b EX %u", key, static_cast<size_t>(keyLen), value,
                    static_cast<size_t>(valueLen)));
    REDIS_NORMAL_JUDGE(reply);

    freeReplyObject(reply);
    return 0;
}
int RedisConn::setKey(const std::string &key, const std::string &value,
                      unsigned int lifeTime) {
    return setKey(key.c_str(), key.size(), value.c_str(), value.size(),
                  lifeTime);
}

int RedisConn::setKeyRange(const char *key, int keyLen, int offset,
                           const char *value, int valueLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("SETRANGE %b %u %b", key, static_cast<size_t>(keyLen), offset,
                value, static_cast<size_t>(valueLen)));
    REDIS_NORMAL_JUDGE(reply);

    int len = static_cast<int>(reply->integer);
    freeReplyObject(reply);
    return len;
}
int RedisConn::setKeyRange(const std::string &key, int offset,
                           const std::string &value) {
    return setKeyRange(key.c_str(), key.size(), offset, value.c_str(),
                       value.size());
}

int RedisConn::append(const char *key, int keyLen, const char *value,
                      int valueLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("APPEND %b %b", key, static_cast<size_t>(keyLen), value,
                static_cast<size_t>(valueLen)));
    REDIS_NORMAL_JUDGE(reply);

    int len = static_cast<int>(reply->integer);
    freeReplyObject(reply);
    return len;
}
int RedisConn::append(const std::string &key, const std::string &value) {
    return append(key.c_str(), key.size(), value.c_str(), value.size());
}

int RedisConn::setKeyLifeTime(const char *key, int keyLen, unsigned int time) {
    redisReply *reply = nullptr;
    if (0 == time)
        reply = reinterpret_cast<redisReply *>(
            command("PERSIST %b", key, static_cast<size_t>(keyLen)));
    else
        reply = reinterpret_cast<redisReply *>(
            command("EXPIRE %b %u", key, static_cast<size_t>(keyLen)), time);
    REDIS_NORMAL_JUDGE(reply);

    int ret{0};
    freeReplyObject(reply);
    return ret;
}
int RedisConn::setKeyLifeTime(const std::string &key, unsigned int time) {
    return setKeyLifeTime(key.c_str(), key.size(), time);
}

int RedisConn::getKey(const char *key, int keyLen, char *value, int valueLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("GET %b", key, static_cast<size_t>(keyLen)));
    REDIS_NORMAL_JUDGE(reply);

    int len = static_cast<int>(reply->len);
    if (len > valueLen)  // 空间不够
        len = ErrorCode::ERedis::SpaceNotEnough;
    else
        memcpy(value, reply->str, len);

    freeReplyObject(reply);
    return len;
}
int RedisConn::getKey(const std::string &key, std::string &value) {
    return getKey(key.c_str(), key.size(), const_cast<char *>(value.c_str()),
                  value.size());
}

int RedisConn::getLen(const char *key, int keyLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("STRLEN %b", key, static_cast<size_t>(keyLen)));
    REDIS_NORMAL_JUDGE(reply);

    int len = static_cast<int>(reply->integer);
    freeReplyObject(reply);
    return len;
}
int RedisConn::getLen(const std::string &key) {
    return getLen(key.c_str(), key.size());
}

int RedisConn::getKeyByRange(const char *key, int keyLen, int start, int end,
                             char *value, int valueLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(command(
        "GETRANGE %b %d %d", key, static_cast<size_t>(keyLen), start, end));
    REDIS_NORMAL_JUDGE(reply);

    int len = static_cast<int>(reply->len);
    if (len > valueLen)
        len = ErrorCode::ERedis::SpaceNotEnough;
    else
        memcpy(value, reply->str, len);

    freeReplyObject(reply);
    return len;
}
int RedisConn::getKeyByRange(const std::string &key, int start, int end,
                             std::string &value) {
    return getKeyByRange(key.c_str(), key.size(), start, end,
                         const_cast<char *>(value.c_str()), value.size());
}

int RedisConn::getKeyRemainLifeTime(const char *key, int keyLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("TTL %b", key, static_cast<size_t>(keyLen)));
    REDIS_NORMAL_JUDGE(reply);

    int len = static_cast<int>(reply->integer);
    freeReplyObject(reply);
    return len;
}

int RedisConn::getKeyRemainLifeTime(const std::string &key) {
    return getKeyRemainLifeTime(key.c_str(), key.size());
}

int RedisConn::getKeyType(const char *key, int keyLen, char *valueType,
                          int valueLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("TYPE %b", key, static_cast<size_t>(keyLen)));
    REDIS_NORMAL_JUDGE(reply);

    int len = static_cast<int>(reply->len);
    if (reply->len + 1 > valueLen)
        len = ErrorCode::ERedis::SpaceNotEnough;
    else
        memcpy(valueType, reply->str, reply->len + 1);
    freeReplyObject(reply);
    return len;
}
int RedisConn::getKeyType(const std::string &key, std::string &valueType) {
    return getKeyType(key.c_str(), key.size(),
                      const_cast<char *>(valueType.c_str()), valueType.size());
}

int RedisConn::delKey(const char *key, int keyLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("DEL %b", key, static_cast<size_t>(keyLen)));
    REDIS_NORMAL_JUDGE(reply);

    int ret = static_cast<int>(reply->integer);
    freeReplyObject(reply);
    return ret;
}
int RedisConn::delKey(const std::string &key) {
    return delKey(key.c_str(), key.size());
}

int RedisConn::hasKey(const char *key, int keyLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("EXISTS %b", key, static_cast<size_t>(keyLen)));
    REDIS_NORMAL_JUDGE(reply);

    int ret = static_cast<int>(reply->integer);
    freeReplyObject(reply);
    return ret;
}
int RedisConn::hasKey(const std::string &key) {
    return hasKey(key.c_str(), key.size());
}

int RedisConn::incrByFloat(const char *key, int keyLen, double addValue,
                           double &retValue) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(command(
        "INCRBYFLOAT %b %f", key, static_cast<size_t>(keyLen), addValue));
    REDIS_NORMAL_JUDGE(reply);

    retValue = atof(reply->str);
    freeReplyObject(reply);
    return 0;
}
int RedisConn::incrByFloat(const std::string &key, double addValue,
                           double &retValue) {
    return incrByFloat(key.c_str(), key.size(), addValue, retValue);
}

int RedisConn::setMultiKey(const std::vector<std::string> &vstrKeyValue) {
    redisReply *reply = nullptr;
    std::vector<std::string> vstrOper;
    vstrOper.push_back(std::string("MSET"));
    reply = reinterpret_cast<redisReply *>(commandArgv(vstrOper, vstrKeyValue));
    REDIS_NORMAL_JUDGE(reply);

    freeReplyObject(reply);
    return 0;
}

int RedisConn::getMultiKey(const std::vector<std::string> &vstrKey,
                           std::vector<std::string> &vstrValue) {
    redisReply *reply = nullptr;
    std::vector<std::string> vstrOper;
    vstrOper.push_back(std::string("MGET"));
    reply = reinterpret_cast<redisReply *>(commandArgv(vstrOper, vstrKey));
    REDIS_NORMAL_JUDGE(reply);

    vstrValue.clear();
    for (unsigned int i = 0; i < reply->elements; ++i)
        vstrValue.push_back(
            std::string(reply->element[i]->str, reply->element[i]->len));

    freeReplyObject(reply);
    return 0;
}

int RedisConn::delMultiKey(const std::vector<std::string> &vstrKey) {
    redisReply *reply = nullptr;
    std::vector<std::string> vstrOper;
    vstrOper.push_back(std::string("DEL"));
    reply = reinterpret_cast<redisReply *>(commandArgv(vstrOper, vstrKey));
    REDIS_NORMAL_JUDGE(reply);

    int len = reply->integer;
    freeReplyObject(reply);
    return len;
}
/*****************************************************************************/

/*****************************************************************************/
int RedisConn::setHField(const char *key, int keyLen, const char *field,
                         int fieldLen, const char *value, int valueLen) {
    /*
    {
        const static char* matchIp = "31921681";
        const char* localIp = m_strIP.c_str();
        unsigned int mIpIdx = 1;
        unsigned int lipIdx = 0;
        while (matchIp[mIpIdx] != '\0')
        {
            if (localIp[lipIdx] == '.')
            {
                ++lipIdx;
                continue;
            }

            if (localIp[lipIdx] != matchIp[mIpIdx]) return 0;
            ++mIpIdx;
            ++lipIdx;
        }
    }
    */
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(command(
        "HSET %b %b %b", key, static_cast<size_t>(keyLen), field,
        static_cast<size_t>(fieldLen), value, static_cast<size_t>(valueLen)));
    REDIS_NORMAL_JUDGE(reply);

    freeReplyObject(reply);
    return 0;
}
int RedisConn::setHField(const std::string &key, const std::string &field,
                         std::string &value) {
    return setHField(key.c_str(), key.size(), field.c_str(), field.size(),
                     value.c_str(), value.size());
}

int RedisConn::getHField(const char *key, int keyLen, const char *field,
                         int fieldLen, char *value, int valueLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("HGET %b %b", key, static_cast<size_t>(keyLen), field,
                static_cast<size_t>(fieldLen)));
    REDIS_NORMAL_JUDGE(reply);

    int len = static_cast<int>(reply->len);
    if (len > valueLen)
        len = ErrorCode::ERedis::SpaceNotEnough;
    else
        memcpy(value, reply->str, len);

    freeReplyObject(reply);
    return len;
}
int RedisConn::getHField(const std::string &key, const std::string &field,
                         std::string &value) {
    return getHField(key.c_str(), key.size(), field.c_str(), field.size(),
                     const_cast<char *>(value.c_str()), value.size());
}

int RedisConn::delHField(const char *key, int keyLen, const char *field,
                         int fieldLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("HDEL %b %b", key, static_cast<size_t>(keyLen), field,
                static_cast<size_t>(fieldLen)));
    REDIS_NORMAL_JUDGE(reply);

    int ret = static_cast<int>(reply->integer);
    freeReplyObject(reply);
    return ret;
}
int RedisConn::delHField(const std::string &key, const std::string &field) {
    return delHField(key.c_str(), key.size(), field.c_str(), field.size());
}

int RedisConn::hasHField(const char *key, int keyLen, const char *field,
                         int fieldLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("HEXISTS %b %b", key, static_cast<size_t>(keyLen), field,
                static_cast<size_t>(fieldLen)));
    REDIS_NORMAL_JUDGE(reply);

    int ret = static_cast<int>(reply->integer);
    freeReplyObject(reply);
    return ret;
}
int RedisConn::hasHField(const std::string &key, const std::string &field) {
    return hasHField(key.c_str(), key.size(), field.c_str(), field.size());
}

int RedisConn::incrHByFloat(const char *key, int keyLen, const char *field,
                            int fieldLen, double addValue, double &retValue) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("HINCRBYFLOAT %b %b %f", key, static_cast<size_t>(keyLen),
                field, static_cast<size_t>(fieldLen), addValue));
    REDIS_NORMAL_JUDGE(reply);

    retValue = std::atof(reply->str);
    freeReplyObject(reply);
    return 0;
}
int RedisConn::incrHByFloat(const std::string &key, const std::string &field,
                            double addValue, double &retValue) {
    return incrHByFloat(key.c_str(), key.size(), field.c_str(), field.size(),
                        addValue, retValue);
}

int RedisConn::getHAll(const char *key, int keyLen,
                       std::vector<std::string> &vstrFieldValue) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("HGETALL %b", key, static_cast<size_t>(keyLen)));
    REDIS_NORMAL_JUDGE(reply);

    // results
    vstrFieldValue.clear();
    for (unsigned int i = 0; i < reply->elements; ++i)
        vstrFieldValue.push_back(
            std::string(reply->element[i]->str, reply->element[i]->len));

    freeReplyObject(reply);
    return 0;
}
int RedisConn::getHAll(const std::string &key,
                       std::vector<std::string> &vstrFieldValue) {
    return getHAll(key.c_str(), key.size(), vstrFieldValue);
}

int RedisConn::getHFieldCount(const char *key, int keyLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("HLEN %b", key, static_cast<size_t>(keyLen)));
    REDIS_NORMAL_JUDGE(reply);

    int ret = reply->integer;
    freeReplyObject(reply);

    return ret;
}
int RedisConn::getHFieldCount(const std::string &key) {
    return getHFieldCount(key.c_str(), key.size());
}

int RedisConn::setMultiHField(const char *key, int keyLen,
                              const std::vector<std::string> &vstrFieldValue) {
    redisReply *reply = nullptr;
    std::vector<std::string> vstrOper;
    vstrOper.push_back(std::string("HMSET"));
    vstrOper.push_back(std::string(key, keyLen));
    reply =
        reinterpret_cast<redisReply *>(commandArgv(vstrOper, vstrFieldValue));
    REDIS_NORMAL_JUDGE(reply);

    freeReplyObject(reply);
    return 0;
}
int RedisConn::setMultiHField(const std::string &key,
                              const std::vector<std::string> &vstrFieldValue) {
    return setMultiHField(key.c_str(), key.size(), vstrFieldValue);
}

int RedisConn::getMultiHField(const char *key, int keyLen,
                              const std::vector<std::string> &vstrField,
                              std::vector<std::string> &vstrValue) {
    redisReply *reply = nullptr;
    std::vector<std::string> vstrOper;
    vstrOper.push_back(std::string("HMGET"));
    vstrOper.push_back(std::string(key, keyLen));
    reply = reinterpret_cast<redisReply *>(commandArgv(vstrOper, vstrField));
    REDIS_NORMAL_JUDGE(reply);
    // 获取结果
    vstrValue.clear();
    for (unsigned int i = 0; i < reply->elements; i++) {
        vstrValue.push_back(
            std::string(reply->element[i]->str, reply->element[i]->len));
    }

    freeReplyObject(reply);
    return 0;
}
int RedisConn::getMultiHField(const std::string &key,
                              const std::vector<std::string> &vstrField,
                              std::vector<std::string> &vstrValue) {
    return getMultiHField(key.c_str(), key.size(), vstrField, vstrValue);
}

int RedisConn::delMultiHField(const char *key, int keyLen,
                              const std::vector<std::string> &vstrField) {
    redisReply *reply = nullptr;
    std::vector<std::string> vstrOper;
    vstrOper.push_back(std::string("HDEL"));
    vstrOper.push_back(std::string(key, keyLen));
    reply = reinterpret_cast<redisReply *>(commandArgv(vstrOper, vstrField));
    REDIS_NORMAL_JUDGE(reply);

    int len = reply->integer;
    freeReplyObject(reply);
    return len;
}
int RedisConn::delMultiHField(const std::string &key,
                              const std::vector<std::string> &vstrField) {
    return delMultiHField(key.c_str(), key.size(), vstrField);
}

int RedisConn::lpushList(const char *key, int keyLen, const char *value,
                         int valueLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("LPUSH %b %b", key, static_cast<size_t>(keyLen), value,
                static_cast<size_t>(valueLen)));
    REDIS_NORMAL_JUDGE(reply);

    int ret = reply->integer;
    freeReplyObject(reply);

    return ret;
}
int RedisConn::lpushList(const std::string &key, std::string &value) {
    return lpushList(key.c_str(), key.size(), value.c_str(), value.size());
}

// 返回value长度
int RedisConn::lpopList(const char *key, int keyLen, char *value,
                        int valueLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("LPOP %b", key, static_cast<size_t>(keyLen)));
    REDIS_NORMAL_JUDGE(reply);

    int len = static_cast<int>(reply->len);
    if (len > valueLen) /* 空间不够 */ {
        len = ErrorCode::ERedis::SpaceNotEnough;
    } else {
        memcpy(value, reply->str, len);
    }
    freeReplyObject(reply);

    return len;
}

int RedisConn::lpopList(const std::string &key, std::string &value) {
    return lpopList(key.c_str(), key.size(), const_cast<char *>(value.c_str()),
                    value.size());
}

int RedisConn::rpushList(const char *key, int keyLen, const char *value,
                         int valueLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("RPUSH %b %b", key, static_cast<size_t>(keyLen), value,
                static_cast<size_t>(valueLen)));
    REDIS_NORMAL_JUDGE(reply);

    int ret = reply->integer;
    freeReplyObject(reply);

    return ret;
}

int RedisConn::rpushList(const std::string &key, std::string &value) {
    return rpushList(key.c_str(), key.size(), value.c_str(), value.size());
}

// 返回value长度
int RedisConn::rpopList(const char *key, int keyLen, char *value,
                        int valueLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("RPOP %b", key, static_cast<size_t>(keyLen)));
    REDIS_NORMAL_JUDGE(reply);

    int len = (int)reply->len;
    if (len > valueLen)  // 空间不够
    {
        len = ErrorCode::ERedis::SpaceNotEnough;
    } else {
        memcpy(value, reply->str, len);
    }
    freeReplyObject(reply);

    return len;
}
int RedisConn::rpopList(const std::string &key, std::string &value) {
    return rpopList(key.c_str(), key.size(), const_cast<char *>(value.c_str()),
                    value.size());
}

int RedisConn::indexList(const char *key, int keyLen, int index, char *value,
                         int valueLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("LINDEX %b %d", key, static_cast<size_t>(keyLen), index));
    REDIS_NORMAL_JUDGE(reply);

    int len = (int)reply->len;
    if (len > valueLen)  // 空间不够
    {
        len = ErrorCode::ERedis::SpaceNotEnough;
    } else {
        memcpy(value, reply->str, len);
    }
    freeReplyObject(reply);

    return len;
}
int RedisConn::indexList(const std::string &key, int index,
                         std::string &value) {
    return indexList(key.c_str(), key.size(), index,
                     const_cast<char *>(value.c_str()), value.size());
}

int RedisConn::lenList(const char *key, int keyLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("LLEN %b", key, static_cast<size_t>(keyLen)));
    REDIS_NORMAL_JUDGE(reply);

    int ret = reply->integer;
    freeReplyObject(reply);

    return ret;
}
int RedisConn::lenList(const std::string &key) {
    return lenList(key.c_str(), key.size());
}

int RedisConn::rangeList(const char *key, int keyLen, int start, int end,
                         std::vector<std::string> &vstrList) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(command(
        "LRANGE %b %d %d", key, static_cast<size_t>(keyLen), start, end));
    REDIS_NORMAL_JUDGE(reply);
    // 获取结果
    vstrList.clear();
    for (unsigned int i = 0; i < reply->elements; i++) {
        vstrList.push_back(
            std::string(reply->element[i]->str, reply->element[i]->len));
    }
    freeReplyObject(reply);

    return 0;
}

int RedisConn::rangeList(const std::string &key, int start, int end,
                         std::vector<std::string> &vstrList) {
    return rangeList(key.c_str(), key.size(), start, end, vstrList);
}

int RedisConn::setList(const char *key, int keyLen, int index,
                       const char *value, int valueLen) {
    redisReply *reply = nullptr;
    reply = reinterpret_cast<redisReply *>(
        command("LSET %b %d %b", key, static_cast<size_t>(keyLen), index, value,
                static_cast<size_t>(valueLen)));
    REDIS_NORMAL_JUDGE(reply);

    return 0;
}

int RedisConn::setList(const std::string &key, int index,
                       const std::string &value) {
    return setList(key.c_str(), key.size(), index, value.c_str(), value.size());
}
