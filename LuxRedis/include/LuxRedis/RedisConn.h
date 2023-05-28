/**
 * @file RedisConn.h
 * @brief
 *
 * @author Lux
 */

#pragma once

#include <LuxUtils/Types.h>
#include <hiredis/hiredis.h>

namespace Lux {

namespace ErrorCode {

/* redis 错误范围 [-101, ..., -200]
 *  封装的 redis api 把错误码作为数据长度返回，因此只能使用负数定义错误码
 */
enum ERedis {
    NormalError = -101,     // redis 常规错误
    NetError = -102,        // 网络连接错误
    SpaceNotEnough = -103,  // 存储空间不够
};
}  // namespace ErrorCode

namespace redis {

class RedisConn {
private:
    redisContext *redisContextPtr_;

    Lux::string IP_;
    uint16_t port_;
    unsigned int timeout_;

    // 禁止拷贝、赋值
    RedisConn(const RedisConn &) = delete;
    RedisConn &operator=(RedisConn &) = delete;

    bool reConnectSvr();
    void *execCommand(const char *cmd, int len);

public:
    RedisConn();
    ~RedisConn();

    /**
     * @brief 连接 redis 服务器
     *
     * @param ip [default 127.0.0.1]
     * @param port [default 6379]
     * @param timeout [default 1500] ms
     * @return true 连接成功
     * @return false 连接失败
     */
    bool connectSvr(const string &ip = "127.0.0.1", uint16_t port = 6379,
                    unsigned int timeout = 1500);

    /**
     * @brief Description: 断开原有的redis连接
     */
    void disconnectSvr();

    // 同步数据到磁盘
    /**
     * @brief fork 一个子进程，同步数据到磁盘，立即返回
     *
     * @return 0 成功； Others 使用 LASTSAVE 命令查看同步结果
     */
    int asynSave();
    /**
     * @brief 阻塞方式，同步数据到磁盘
     * @return 0 成功, Others 当 key 较多时，很慢
     */
    int save();

    /**
     * @brief Redis 原始接口，若失败会尝试重连一次
     * @return void* , 一般为 redisReply*
     */
    void *command(const char *format, ...);
    void *commandArgv(const std::vector<std::string> &vstrOper,
                      const std::vector<std::string> &vstrParam);

    // ------------------- String -------------------
    /**
     * @brief 设置一对
     * key:value，若已存在，则直接覆盖；（数值类型得格式化成string）
     *
     * @param key
     * @param keyLen
     * @param value
     * @param valueLen
     * @param lifeTime second；0 代表永不删除
     * @return int 0 - success
     */
    int setKey(const char *key, int keyLen, const char *value, int valueLen,
               unsigned int lifeTime = 0);
    int setKey(const std::string &key, const std::string &value,
               unsigned int lifeTime = 0);
    /**
     * @brief 用 value 参数覆写（overwrite）给定 key 所存储的字符串值，从偏移量
     * offset 开始
     *
     * @param key
     * @param keyLen
     * @param offset 0 开始
     * @param value
     * @param valueLen
     * @return int 修改后 value 的长度
     * @others 如果给定 key 原来储存的字符串长度比偏移量小，
                那么原字符和偏移量之间的空白将用零字节("\x00")来填充
     */
    int setKeyRange(const char *key, int keyLen, int offset, const char *value,
                    int valueLen);
    int setKeyRange(const std::string &key, int offset,
                    const std::string &value);

    /**
     * @brief 在原有的 key 追加，若 key 不存在，相当于 setkey
     *
     * @param key
     * @param keyLen
     * @param value
     * @param valueLen
     * @return int 修改后 value 的长度
     */
    int append(const char *key, int keyLen, const char *value, int valueLen);
    int append(const std::string &key, const std::string &value);

    /**
     * @brief 设置 key 的生存时间
     *
     * @param key
     * @param keyLen
     * @param time seconds, 0 - 代表永不删除
     * @return int 0 - success
     */
    int setKeyLifeTime(const char *key, int keyLen, unsigned int time = 0);
    int setKeyLifeTime(const std::string &key, unsigned int time = 0);

    /**
     * @brief 获取 key 对应的 value
     *
     * @param key
     * @param keyLen
     * @param value 用于接收 key 对应的 value,若存储空间不够，返回失败
     * @param valueLen
     * @return int 返回长度，0 表示 key 不存在
     */
    int getKey(const char *key, int keyLen, char *value, int valueLen);
    int getKey(const std::string &key, std::string &value);

    /**
     * @brief 获取 key 对应的 value 的长度
     *  STRLEN key
     * @param key
     * @param keyLen
     * @return int 返回长度，0代表不存在
     */
    int getLen(const char *key, int keyLen);
    int getLen(const std::string &key);

    /**
     * @brief 获取 key 对应 [start end] 区间的数据
     *
     * @param key
     * @param keyLen
     * @param start -1代表最后1个，-2代表倒数第2个.
     * @param end -1代表最后1个，-2代表倒数第2个
     * @param value 获取的数据
     * @param valueLen
     * @return int 返回长度，0代表key不存在或end在start前;
     * 通过保证子字符串的值域(range)不超过实际字符串的值域来处理超出范围的值域请求
     */
    int getKeyByRange(const char *key, int keyLen, int start, int end,
                      char *value, int valueLen);
    int getKeyByRange(const std::string &key, int start, int end,
                      std::string &value);
    /**
     * @brief 获取 key 的剩余时间
     *
     * @param key
     * @param keyLen
     * @return int -2:key 不存在，-1:永久，其它则返回剩余时间(秒)
     */
    int getKeyRemainLifeTime(const char *key, int keyLen);
    int getKeyRemainLifeTime(const std::string &key);

    /**
     * @brief 获取 key 的类型
     *
     * @param key
     * @param keyLen
     * @param valueType none(key不存在) string(字符串)	list(列表)
     * set(集合) zset(有序集) hash(哈希表)
     * @param valueLen
     * @return int
     */
    int getKeyType(const char *key, int keyLen, char *valueType, int valueLen);
    int getKeyType(const std::string &key, std::string &valueType);

    /**
     * @brief delete key
     *
     * @param key
     * @param keyLen
     * @return int 1 - delete key successfully, 0 - key don't exist
     */
    int delKey(const char *key, int keyLen);
    int delKey(const std::string &key);

    /**
     * @brief 判断 key 是否存在
     *
     * @param key
     * @param keyLen
     * @return int key存在返回 1，不存在返回 0
     */
    int hasKey(const char *key, int keyLen);
    int hasKey(const std::string &key);

    /**
     * @brief 对 key 做加法
     *
     * @param key
     * @param keyLen
     * @param addValue 需要相加的值（可为负数）
     * @param retValue 用于接收相加后的结果
     * @return int 执行成功返回 0；
     * 如果 key 不存在，那么会先将 key 的值设为 0 ，再执行加法操作
     */
    int incrByFloat(const char *key, int keyLen, double addValue,
                    double &retValue);
    int incrByFloat(const std::string &key, double addValue, double &retValue);

    /**
     * @brief MSET KEY1 VALUE1 [KEY2 VALUE2 ...]
     *
     * @param vstrKeyValue
     * @return int 0 - success
     */
    int setMultiKey(const std::vector<std::string> &vstrKeyValue);

    /**
     * @brief MGET KEY1 [KEY2 ...]
     *
     * @param vstrKey The keys to get.
     * @param vstrValue The valuse to be saved.
     * @return int
     */
    int getMultiKey(const std::vector<std::string> &vstrKey,
                    std::vector<std::string> &vstrValue);

    /**
     * @brief DEL KEY1 [KEY2 KEY3]
     *
     * @param vstrKey
     * @return int
     */
    int delMultiKey(const std::vector<std::string> &vstrKey);

    // hash表
    /**
     * @brief 在哈希表 key 设置 field:value
     *
     * @param key
     * @param keyLen
     * @param field
     * @param fieldLen
     * @param value
     * @param valueLen
     * @return int 0 - success
     */
    int setHField(const char *key, int keyLen, const char *field, int fieldLen,
                  const char *value, int valueLen);
    int setHField(const std::string &key, const std::string &field,
                  std::string &value);

    /**
     * @brief hash 表中获取 field 的值
     *
     * @param key
     * @param keyLen
     * @param field
     * @param fieldLen
     * @param value
     * @param valueLen
     * @return int
     */
    int getHField(const char *key, int keyLen, const char *field, int fieldLen,
                  char *value, int valueLen);
    int getHField(const std::string &key, const std::string &field,
                  std::string &value);

    /**
     * @brief 删除 hash 表中的 field
     *
     * @param key
     * @param keyLen
     * @param field
     * @param fieldLen
     * @return int
     */
    int delHField(const char *key, int keyLen, const char *field, int fieldLen);
    int delHField(const std::string &key, const std::string &field);

    /**
     * @brief 判断 hash 表中是否存在 field
     *
     * @param key
     * @param keyLen
     * @param field
     * @param fieldLen
     * @return int
     */
    int hasHField(const char *key, int keyLen, const char *field, int fieldLen);
    int hasHField(const std::string &key, const std::string &field);

    /**
     * @brief 将 hash 表中的 field 的值增加 addValue，retValue
     * 用于接收增加后的值
     *
     * @param key
     * @param keyLen
     * @param field
     * @param fieldLen
     * @param addValue
     * @param retValue
     * @return int
     */
    int incrHByFloat(const char *key, int keyLen, const char *field,
                     int fieldLen, double addValue, double &retValue);
    int incrHByFloat(const std::string &key, const std::string &field,
                     double addValue, double &retValue);

    /**
     * @brief 获取 hash 表中的所有的 field
     *
     * @param key
     * @param keyLen
     * @param vstrFieldValue
     * @return int
     */
    int getHAll(const char *key, int keyLen,
                std::vector<std::string> &vstrFieldValue);
    int getHAll(const std::string &key,
                std::vector<std::string> &vstrFieldValue);

    /**
     * @brief  统计 hash 表中的 field 的个数
     *
     * @param key
     * @param keyLen
     * @return int
     */
    int getHFieldCount(const char *key, int keyLen);
    int getHFieldCount(const std::string &key);

    /**
     * @brief 设置 hash 表中的多个 field
     *
     * @param key
     * @param keyLen
     * @param vstrField
     * @return int
     */
    int setMultiHField(const char *key, int keyLen,
                       const std::vector<std::string> &vstrFieldValue);
    int setMultiHField(const std::string &key,
                       const std::vector<std::string> &vstrFieldValue);

    /**
     * @brief 获取 hash 表中的多个 field
     *
     * @param key
     * @param keyLen
     * @param vstrField
     * @param vstrValue
     * @return int
     */
    int getMultiHField(const char *key, int keyLen,
                       const std::vector<std::string> &vstrField,
                       std::vector<std::string> &vstrValue);
    int getMultiHField(const std::string &key,
                       const std::vector<std::string> &vstrField,
                       std::vector<std::string> &vstrValue);

    /**
     * @brief 删除 hash 表中的多个 field
     *
     * @param key
     * @param keyLen
     * @param vstrField
     * @return int
     */
    int delMultiHField(const char *key, int keyLen,
                       const std::vector<std::string> &vstrField);
    int delMultiHField(const std::string &key,
                       const std::vector<std::string> &vstrField);

    // 列表
    /**
     * @brief 左侧插入 value; 只支持插入一个数据
     *      LPUSH list value
     * @param key 待插入的 list 名称
     * @param keyLen
     * @param value To be inserted
     * @param valueLen
     * @return int
     */
    int lpushList(const char *key, int keyLen, const char *value, int valueLen);
    int lpushList(const std::string &key, std::string &value);

    /**
     * @brief 左侧删除一个节点
     *
     * @param key 待操作的list 名称
     * @param keyLen
     * @param value 删除掉的节点值
     * @param valueLen
     * @return int
     */
    int lpopList(const char *key, int keyLen, char *value, int valueLen);
    int lpopList(const std::string &key, std::string &value);

    /**
     * @brief 右侧插入 value; 只支持插入一个数据
     *      RPUSH list value
     * @param key 待插入的 list 名称
     * @param keyLen
     * @param value To be inserted
     * @param valueLen
     * @return int
     */
    int rpushList(const char *key, int keyLen, const char *value, int valueLen);
    int rpushList(const std::string &key, std::string &value);

    /**
     * @brief 右侧删除一个节点
     *
     * @param key 待操作的list 名称
     * @param keyLen
     * @param value 删除掉的节点值
     * @param valueLen
     * @return int
     */
    int rpopList(const char *key, int keyLen, char *value, int valueLen);
    int rpopList(const std::string &key, std::string &value);

    /**
     * @brief 获取从左侧起下标为 index([0, ..]) 个元素的 value
     *
     * @param key 待操作的list 名称
     * @param keyLen
     * @param index 下标
     * @param value 获取到的 value
     * @param valueLen
     * @return int
     */
    int indexList(const char *key, int keyLen, int index, char *value,
                  int valueLen);
    int indexList(const std::string &key, int index, std::string &value);

    /**
     * @brief 获取当前 list 的长度
     *
     * @param key
     * @param keyLen
     * @return int
     */
    int lenList(const char *key, int keyLen);
    int lenList(const std::string &key);

    /**
     * @brief 获取 list 中 [start, end] 的元素值
     *
     * @param key
     * @param keyLen
     * @param start
     * @param end
     * @param vstrList
     * @return int 0 - successful
     */
    int rangeList(const char *key, int keyLen, int start, int end,
                  std::vector<std::string> &vstrList);

    int rangeList(const std::string &key, int start, int end,
                  std::vector<std::string> &vstrList);
    /**
     * @brief 将 list 中下标为 index 的元素更新为 value
     *
     * @param key
     * @param keyLen
     * @param index
     * @param value
     * @param valueLen
     * @return int 0 - Set it successfully.
     */
    int setList(const char *key, int keyLen, int index, const char *value,
                int valueLen);
    int setList(const std::string &key, int index, const std::string &value);
};
}  // namespace redis
}  // namespace Lux
