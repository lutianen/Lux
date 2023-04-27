#include <LuxLog/Logger.h>
#include <LuxMySQL/MySQLConn.h>
#include <LuxMySQL/MySQLConnPool.h>

#include <thread>

using namespace Lux::mysql;

int main() {
    Lux::Logger::setLogLevel(Lux::Logger::LogLevel::DEBUG);

    auto connpool = MySQLConnPool::getInstance();
    connpool->init(10, "192.168.132.132", 3306, "lutianen", "lutianen",
                   "Luxdb");

    std::thread t1([&] {
        MYSQL* mysql = nullptr;
        MySQLConn conn(mysql, connpool);
        conn.execute("SELECT username, mail FROM user;");

        LOG_INFO << conn.stmtRes_.numFields_;
        for (auto& row : conn.stmtRes_.rows_) {
            for (int i = 0; i < conn.stmtRes_.numFields_; i++) {
                LOG_INFO << row[i];
            }
        }
    });

    t1.join();

    return 0;
}
