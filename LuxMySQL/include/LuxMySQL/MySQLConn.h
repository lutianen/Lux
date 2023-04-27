/**
 * @file MySQLConn.h
 * @brief
 *
 * @author Lux
 */

#pragma once

#include <LuxMySQL/MySQLConnPool.h>
#include <mysql/mysql.h>

namespace Lux {
namespace mysql {

/**
 * @brief
 *
 */
class MySQLConn {
    MySQLConn(const MySQLConn&) = delete;
    MySQLConn& operator=(MySQLConn&) = delete;

public:
    struct StmtRes {
        /* return code: 0 - successful, others - failed */
        unsigned int rc_;
        // 从表中检索完整的结果集 - 如果不为 nullptr，表示执行成功
        MYSQL_RES* res_;
        // 返回结果集中的列数
        int numFields_;
        // 返回所有字段结构的数组
        MYSQL_FIELD* fields_;
        /// vector<char **> rows_
        ///  查询结果集中的一行数据的集合
        std::vector<MYSQL_ROW> rows_;
    };

    StmtRes stmtRes_;

private:
    bool connected_;
    MYSQL* con_;
    MySQLConnPool* pool_;

public:
    MySQLConn(MYSQL* conn, MySQLConnPool* pool);
    ~MySQLConn() {
        pool_->releaseConnection(con_);
        connected_ = false;
    }

    void execute(const char* query);
};

}  // namespace mysql
}  // namespace Lux
