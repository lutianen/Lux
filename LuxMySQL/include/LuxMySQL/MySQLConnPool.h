/**
 * @file MySQLConnPool.h
 * @brief
 *
 * @author Lux
 */

#pragma once

#include <LuxUtils/Condition.h>
#include <LuxUtils/Mutex.h>
#include <LuxUtils/Types.h>
#include <mysql/mysql.h>

#include <queue>

#include "LuxLog/Logger.h"

namespace Lux {
namespace mysql {
class MySQLConnPool {
    MySQLConnPool(const MySQLConnPool&) = delete;
    MySQLConnPool& operator=(MySQLConnPool&) = delete;

private:
    int capicity_;  // 数据库连接池的最大容量
    int curSize_;   // 数据库连接池当前连接数
    int leftSize_;  // 数据库连接池中剩余的连接数

    MutexLock mutex_;                    // 互斥量
    Condition cond_ GUARDED_BY(mutex_);  // 条件变量，用于多线程的同步
    std::queue<MYSQL*> pool_;  // 连接池，使用 queue 实现，也可以采用 list

    /// The state of connecting to database
    /// false - disconnected, true - connected
    bool state_;
    string ipAddr_;  // 数据库 ip 地址
    uint16_t port_;  // 数据库 port
    string user_;    // 数据库 用户
    string passwd_;  // 数据库 用户对应的密码
    string db_;      // 数据库名字

    // The flag of auto commit;
    // false - disEnable; true - enable
    bool autoCommit_;

public:
    MySQLConnPool(int capicity, const string& ipAddr, uint16_t port,
                  const string& user, const string& passwd, const string& db,
                  bool autoCommitOpt = false);
    MySQLConnPool();
    ~MySQLConnPool() { destroyPool(); }

    void init(int capcity, const string& ipAddr, uint16_t port,
              const string& user, const string& passwd, const string& db,
              bool autoCommit = false, const char* charsest = "utf8mb4");

    inline bool state() const { return state_; }
    void setChacter(MYSQL* mysql, const char* charset);

    MYSQL* getConnecton();
    bool releaseConnection(MYSQL* conn);
    void destroyPool();
    int leftSize() const { return leftSize_; }

    static MySQLConnPool* getInstance();
};

}  // namespace mysql
}  // namespace Lux
