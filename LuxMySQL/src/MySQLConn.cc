/**
 * @file MySQLConn.cc
 * @brief
 *
 * @author Lux
 */

#include <LuxMySQL/MySQLConn.h>
#include <mysql/mysql.h>

#include <algorithm>
#include <cctype>

using namespace Lux;
using namespace Lux::mysql;

MySQLConn::MySQLConn(MYSQL* conn, MySQLConnPool* pool) {
    conn = pool->getConnecton();

    connected_ = conn != nullptr;
    con_ = conn;
    pool_ = pool;

    stmtRes_.rc_ = 0;
    stmtRes_.fields_ = nullptr;
    stmtRes_.numFields_ = 0;
    stmtRes_.res_ = nullptr;
    stmtRes_.rows_.clear();
}

void MySQLConn::execute(const char* query) {
    if (false == connected_) {
        LOG_ERROR << "Current connection is no't connected.";
        return;
    }

    string temp = query;
    string stmtType = temp.substr(0, 6);
    std::transform(stmtType.begin(), stmtType.end(), stmtType.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    if (mysql_query(con_, query)) {
        LOG_ERROR << mysql_error(con_);
    }

    stmtRes_.rc_ = mysql_errno(con_);
    if (stmtRes_.rc_ == 0) {
        if (stmtType == "SELECT") {
            auto result = mysql_store_result(con_);
            stmtRes_.res_ = result;
            stmtRes_.numFields_ = mysql_num_fields(result);
            stmtRes_.fields_ = mysql_fetch_field(stmtRes_.res_);
            while (MYSQL_ROW row = mysql_fetch_row(stmtRes_.res_))
                stmtRes_.rows_.push_back(row);
        }
    } else {
        LOG_ERROR << mysql_error(con_);
    }
}
