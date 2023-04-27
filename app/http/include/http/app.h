/**
 * @file app.h
 * @brief
 *
 * @author Lux
 */

#pragma once

#include <LuxLog/Logger.h>
#include <LuxMySQL/MySQLConnPool.h>
#include <http/HttpRequest.h>
#include <http/HttpResponse.h>
#include <http/HttpServer.h>
#include <mysql/mysql.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <map>
#include <regex>

namespace Lux {
namespace http {

class Application {
public:
    static const int FILENAME_LEN = 200;

private:
    Lux::polaris::EventLoop* loop_;
    HttpServer server_;
    int numThreads_;

    char realFile_[FILENAME_LEN];
    string serverPath_;
    struct stat fileStat_;
    char* fileAddr_;

    string dbIpAddr_;
    uint16_t dbPort_;
    // 数据用户名
    string dbUser_;
    // 密码
    string dbPasswd_;
    // 数据库名
    string dbName_;
    // 数据库连接池
    mysql::MySQLConnPool* connPool_;

public:
    Application(Lux::polaris::EventLoop* loop,
                const Lux::polaris::InetAddress& listenAddr, const string& name,
                const string& root, const string& dbIpAddr, uint16_t dbPort,
                const string& dbUser, const string& dbPasswd,
                const string& dbName);

    void onRequest(const HttpRequest& req, HttpResponse* resp);

    void setNumThreads(int num) { server_.setThreadNum(num); }
    void start() { server_.start(); }

private:
    string getHtml();
};
}  // namespace http
}  // namespace Lux