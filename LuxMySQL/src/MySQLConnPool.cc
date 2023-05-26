/**
 * @file MySQLConnPool.cc
 * @brief
 *
 * @author Lux
 */

#include <LuxMySQL/MySQLConnPool.h>
#include <mysql/mysql.h>

#include <cstdint>
#include <cstdlib>

using namespace Lux;
using namespace Lux::mysql;

// 单例模式
// 利用局部静态变量的特性
Lux::mysql::MySQLConnPool* Lux::mysql::MySQLConnPool::getInstance() {
    static Lux::mysql::MySQLConnPool connPool;
    return &connPool;
}

MySQLConnPool::MySQLConnPool(int capicity, const string& ipAddr, uint16_t port,
                             const string& user, const string& passwd,
                             const string& db, bool autoCommit)
    : capicity_(capicity),
      curSize_(0),
      leftSize_(0),
      mutex_(),
      cond_(mutex_),
      pool_(),
      state_(false),
      ipAddr_(ipAddr),
      port_(port),
      user_(user),
      passwd_(passwd),
      db_(db),
      autoCommit_(autoCommit) {}

MySQLConnPool::MySQLConnPool()
    : capicity_(0),
      curSize_(0),
      leftSize_(0),
      mutex_(),
      cond_(mutex_),
      state_(false) {
    LOG_DEBUG << __func__;
}

/**
 * @brief Set the Chacter object
 *
 * @param mysql MYSQL*
 * @param charset - "gbk" / "utf8" / "utf8mb4 (default)"
 */
void MySQLConnPool::setChacter(MYSQL* mysql, const char* charset) {
    if (mysql_set_character_set(mysql, charset) != 0) {
        LOG_ERROR << "Failed to set character: " << charset
                  << mysql_error(mysql);
        exit(1);
    }
}

void MySQLConnPool::init(int capcity, const string& ipAddr, uint16_t port,
                         const string& user, const string& passwd,
                         const string& db, bool autoCommit,
                         const char* charset) {
    ipAddr_ = ipAddr;
    port_ = port;
    user_ = user;
    passwd_ = passwd;
    db_ = db;

    for (int i = 0; i < capcity; ++i) {
        MYSQL* mysql = nullptr;
        mysql = mysql_init(mysql);
        if (nullptr == mysql) {
            /**
               1. MySQL 服务器未运行或无法连接：
                    确保 MySQL 服务器正在运行，
                    并且您的代码中指定的主机名、端口、用户名和密码正确。
               2. MySQL C API 库未正确安装：
                    请检查您的系统中是否正确安装了 MySQL C API库。
                    如果没有，请安装正确的库文件。
               3. 缺少必要的库文件：
                    如果您使用的是动态链接库，则您的系统中必须存在所需的库文件。
                    请确保您的系统中安装了所有必要的库文件，
                    包括 MySQL C API 库和其依赖的库文件。
               4. 内存分配错误：
                    如果您的系统内存不足，或者您的代码中存在其他内存分配问题，
                    则可能会导致 mysql_init 初始化失败。
             */
            LOG_ERROR << "return code: " << -1
                      << "\nError message: Initialize MySQL failed.\n";

            exit(1);
        }

        if (mysql_real_connect(mysql, ipAddr_.c_str(), user_.c_str(),
                               passwd_.c_str(), db_.c_str(), port_, nullptr,
                               0) != mysql) {
            LOG_ERROR << "return code: " << mysql_errno(mysql)
                      << "\nError message: " << mysql_error(mysql);

            mysql_close(mysql);
            exit(1);
        }

        // auto commit
        autoCommit_ = autoCommit;
        if (mysql_autocommit(mysql, autoCommit_) != false) {
            LOG_ERROR << "return code: " << mysql_errno(mysql)
                      << "\nError message: " << mysql_error(mysql);

            mysql_close(mysql);
            mysql = nullptr;
            exit(1);
        }

        // set character for every connection
        setChacter(mysql, charset);

        // push conn into que
        {
            MutexLockGuard lck(mutex_);
            pool_.push(mysql);
            ++leftSize_;
        }
    }

    state_ = true;
    capicity_ = leftSize_;
}

/**
 * @brief
 * 当有请求到来时，从数据库连接池中返回一个可用连接，并更新使用和空闲连接数
 *
 * @return MYSQL*
 */
MYSQL* MySQLConnPool::getConnecton() {
    MYSQL* conn = nullptr;
    if (0 == pool_.size()) return nullptr;

    {
        MutexLockGuard lock(mutex_);
        while (pool_.empty()) {
            cond_.wait();
        }

        conn = pool_.front();
        pool_.pop();
        --leftSize_;
        ++curSize_;
    }

    return conn;
}

/**
 * @brief 释放当前使用的连接，通过将当前连接放入数据库连接池实现
 *
 * @param conn
 * @return bool
 */
bool MySQLConnPool::releaseConnection(MYSQL* conn) {
    if (nullptr == conn) return false;

    {
        MutexLockGuard lock(mutex_);
        pool_.push(conn);
        ++leftSize_;
        --curSize_;
    }

    cond_.notify();
    return true;
}

/**
 * @brief 销毁当前连接池，即关闭连接池中所有的 MYSLQ*
 *
 */
void MySQLConnPool::destroyPool() {
    MutexLockGuard lock(mutex_);

    if (pool_.size() > 0) {
        while (!pool_.empty()) {
            MYSQL* it = pool_.front();
            pool_.pop();
            mysql_close(it);
        }

        curSize_ = 0;
        leftSize_ = 0;
    }

    state_ = false;
}
