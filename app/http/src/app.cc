#include <LuxLog/Logger.h>
#include <fcntl.h>
#include <http/HttpRequest.h>
#include <http/HttpResponse.h>
#include <http/HttpServer.h>
#include <http/MysqlConn.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// #include <LuxUtils/LuxINI.hpp>
#include <mysql/mysql.h>

#include <fstream>
#include <map>
#include <regex>

using namespace Lux;
using namespace Lux::polaris;
using namespace Lux::http;

extern char favicon[555];
bool benchmark = false;

// username : <mail, password>
std::map<string, std::pair<string, string>> users;

class Application {
public:
    static const int FILENAME_LEN = 200;

private:
    EventLoop* loop_;
    HttpServer server_;
    int numThreads_;

    char realFile_[FILENAME_LEN];
    string serverPath_;
    struct stat fileStat_;
    char* fileAddr_;

    // 数据用户名
    string user_;
    // 密码
    string passwd_;
    // 数据库名
    string dbName_;

public:
    Application(EventLoop* loop, const InetAddress& listenAddr,
                const string& name, const string& root, const string& user,
                const string& passwd, const string& dbName)
        : loop_(loop),
          server_(loop, listenAddr, name),
          numThreads_(0),
          realFile_(),
          serverPath_(root),
          fileStat_(),
          fileAddr_(nullptr),
          user_(user),
          passwd_(passwd),
          dbName_(dbName) {
        memZero(realFile_, FILENAME_LEN);

        server_.setHttpCallback(
            std::bind(&Application::onRequest, this, _1, _2));

        MYSQL* mysql = nullptr;
        mysql = mysql_init(mysql);
        if (nullptr == mysql) {
            LOG_ERROR << "MySQL init Error";
            exit(1);
        }

        mysql = mysql_real_connect(mysql, "localhost", user_.c_str(),
                                   passwd_.c_str(), "Luxdb", 3306, nullptr, 0);
        if (nullptr == mysql) {
            LOG_ERROR << "MySQL init Error";
            exit(1);
        }

        if (mysql_query(mysql, "SELECT username, mail, passwd FROM user")) {
            LOG_ERROR << "SELECT error: " << mysql_error(mysql);
        }

        // 从表中检索完整的结果集
        MYSQL_RES* result = mysql_store_result(mysql);

        // 返回结果集中的列数
        int numFields = mysql_num_fields(result);

        // 返回所有字段结构的数组
        MYSQL_FIELD* fields = mysql_fetch_field(result);

        while (MYSQL_ROW row = mysql_fetch_row(result)) {
            users[row[0]] = {row[1], row[2]};
        }

        mysql_close(mysql);
    }
    void onRequest(const HttpRequest& req, HttpResponse* resp) {
        LOG_INFO << "Headers " << req.methodString() << " " << req.path();

        if (req.path() == "/" || req.path() == "/index.html") {
            resp->setStatusCode(HttpResponse::HttpStatusCode::k200Ok);
            resp->setStatusMessage("OK");
            resp->setContentType("text/html");
            resp->addHeader("Server", "Lux polaris");

            strcpy(realFile_, serverPath_.c_str());
            int len = strlen(realFile_);
            const char* index = "/index.html";
            strcat(realFile_, index);

            resp->setBody(getHtml());
        } else if (req.path() == "/register") {
            LOG_INFO << req.path();
            resp->setStatusCode(HttpResponse::HttpStatusCode::k200Ok);
            resp->setStatusMessage("OK");
            resp->setContentType("text/html");
            resp->addHeader("Server", "Lux polaris");
            string body = req.body();

            if (req.method() == HttpRequest::Method::kPost && !body.empty()) {
                // 处理注册信息 - user, mail, password
                string username, mail, password;
                std::regex pattern(
                    "Username=([0-9a-zA-Z_]+)&email=([0-9a-zA-Z]+\\%40[a-zA-Z]+"
                    "\\."
                    "com)&password=([0-9a-zA-Z]+)");
                for (std::sregex_iterator
                         iter(body.begin(), body.end(), pattern),
                     iterend;
                     iter != iterend; ++iter) {
                    username = iter->str(1);
                    mail = iter->str(2);
                    password = iter->str(3);
                }

                char* sqlInsert = new char[200];
                strcpy(sqlInsert,
                       "INSERT INTO user(username, mail, passwd) VALUES(");
                strcat(sqlInsert, "'");
                strcat(sqlInsert, username.c_str());
                strcat(sqlInsert, "', '");
                strcat(sqlInsert, mail.c_str());
                strcat(sqlInsert, "', '");
                strcat(sqlInsert, password.c_str());
                strcat(sqlInsert, "')");

                if (users.find(username) == users.end()) {
                    MYSQL* mysql = nullptr;
                    mysql = mysql_init(mysql);
                    if (nullptr == mysql) {
                        LOG_ERROR << "MySQL init Error";
                        delete[] sqlInsert;
                        sqlInsert = nullptr;
                        exit(1);
                    }

                    mysql = mysql_real_connect(mysql, "localhost",
                                               user_.c_str(), passwd_.c_str(),
                                               "Luxdb", 3306, nullptr, 0);
                    if (nullptr == mysql) {
                        LOG_ERROR << "MySQL init Error";
                        delete[] sqlInsert;
                        sqlInsert = nullptr;
                        exit(1);
                    }

                    int ret = mysql_query(mysql, sqlInsert);
                    users[username] = {mail, password};

                    // 成功
                    if (!ret) {
                        strcpy(realFile_, serverPath_.c_str());
                        int len = strlen(realFile_);
                        const char* index = "/welcome.html";
                        strcat(realFile_, index);
                        delete[] sqlInsert;
                        sqlInsert = nullptr;
                        mysql_close(mysql);
                    } else {
                        LOG_ERROR << "SELECT error: " << mysql_error(mysql);
                        delete[] sqlInsert;
                        sqlInsert = nullptr;
                        mysql_close(mysql);
                    }
                } else {
                    strcpy(realFile_, serverPath_.c_str());
                    int len = strlen(realFile_);
                    const char* index = "/registerFailed.html";
                    strcat(realFile_, index);
                }
            } else {
                strcpy(realFile_, serverPath_.c_str());
                int len = strlen(realFile_);
                const char* index = "/register.html";
                strcat(realFile_, index);
            }

            resp->setBody(getHtml());
        } else if (req.path() == "/welcome") {
            resp->setStatusCode(HttpResponse::HttpStatusCode::k200Ok);
            resp->setStatusMessage("OK");
            resp->setContentType("text/html");
            resp->addHeader("Server", "Lux polaris");

            strcpy(realFile_, serverPath_.c_str());
            int len = strlen(realFile_);
            const char* index = "/welcome.html";
            strcat(realFile_, index);

            resp->setBody(getHtml());

        } else if (req.path().find(".jpg") != string::npos) {
            resp->setStatusCode(HttpResponse::HttpStatusCode::k200Ok);
            resp->setStatusMessage("OK");
            resp->setContentType("image/jpg");
            resp->addHeader("Server", "Lux polaris");

            strcpy(realFile_, serverPath_.c_str());
            int len = strlen(realFile_);
            const char* index = req.path().c_str();
            strcat(realFile_, index);

            LOG_INFO << realFile_;

            // NO resource
            if (stat(realFile_, &fileStat_) < 0) return;
            // FORBIDDEN REQUEST
            if (!(fileStat_.st_mode & S_IROTH)) return;
            // BAD_REQUEST
            if (S_ISDIR(fileStat_.st_mode)) return;

            std::ifstream ifs(realFile_, std::ios::ios_base::binary);
            if (!ifs.is_open()) {
                LOG_ERROR << "Open " << realFile_ << " failed";
                resp->setBody("");
            } else {
                //将文件读入到ostringstream对象buf中
                std::ostringstream buf{};
                char ch;
                while (buf && ifs.get(ch)) buf.put(ch);

                resp->setBody(buf.str());
            }
        } else if (req.method() == HttpRequest::Method::kGet &&
                   req.path().find("/login") != std::string::npos) {
            LOG_INFO << req.path();
            LOG_INFO << req.query();

            std::regex pattern(
                "\\?Username=([0-9a-zA-Z]+)&password=([0-9a-zA-Z]+)");

            string query = req.query(), username, passwd;
            for (std::sregex_iterator iter(query.begin(), query.end(), pattern),
                 iterEnd;
                 iter != iterEnd; ++iter) {
                username = iter->str(1);
                passwd = iter->str(2);
            }

            // 利用全局缓存，不用连接数据库
            // FIXME 使用 Redis 缓存
            if (users.find(username) != users.end()) {
                if (users.at(username).second == passwd) {
                    resp->setStatusCode(HttpResponse::HttpStatusCode::k200Ok);
                    resp->setStatusMessage("OK");
                    resp->setContentType("text/html");
                    resp->addHeader("Server", "Lux polaris");

                    strcpy(realFile_, serverPath_.c_str());
                    int len = strlen(realFile_);
                    const char* index = "/welcome.html";
                    strcat(realFile_, index);

                    resp->setBody(getHtml());
                } else {
                    resp->setStatusCode(HttpResponse::HttpStatusCode::k200Ok);
                    resp->setStatusMessage("OK");
                    resp->setContentType("text/html");
                    resp->addHeader("Server", "Lux polaris");

                    strcpy(realFile_, serverPath_.c_str());
                    int len = strlen(realFile_);
                    const char* index = "/loginFailed.html";
                    strcat(realFile_, index);

                    resp->setBody(getHtml());
                }
            } else {
                resp->setStatusCode(HttpResponse::HttpStatusCode::k200Ok);
                resp->setStatusMessage("OK");
                resp->setContentType("text/html");
                resp->addHeader("Server", "Lux polaris");

                strcpy(realFile_, serverPath_.c_str());
                int len = strlen(realFile_);
                const char* index = "/loginFailed.html";
                strcat(realFile_, index);

                resp->setBody(getHtml());
            }

        } else {
            resp->setStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
            resp->setStatusMessage("Not Found");

            resp->setContentType("text/html");
            resp->addHeader("Server", "Lux polaris");

            strcpy(realFile_, serverPath_.c_str());
            int len = strlen(realFile_);
            strcat(realFile_, "/404.html");

            resp->setBody(getHtml());

            resp->setCloseConnection(true);
        }
        // FILE REQUEST
        return;
    }

    void setNumThreads(int num) { server_.setThreadNum(num); }
    void start() { server_.start(); }

private:
    string getHtml() {
        string ret{};

        // NO resource
        if (stat(realFile_, &fileStat_) < 0) return ret;
        // FORBIDDEN REQUEST
        if (!(fileStat_.st_mode & S_IROTH)) return ret;
        // BAD_REQUEST
        if (S_ISDIR(fileStat_.st_mode)) return ret;

        std::ifstream ifs(realFile_, std::ios::ios_base::binary);
        if (!ifs.is_open()) {
            LOG_ERROR << "Open " << realFile_ << " failed";
        } else {
            //将文件读入到ostringstream对象buf中
            std::ostringstream buf{};
            char ch;
            while (buf && ifs.get(ch)) buf.put(ch);

            return buf.str();
        }
        return ret;
    }
};

int main(int argc, char* argv[]) {
    int numThreads = 0;
    if (argc > 1) {
        benchmark = true;
        Logger::setLogLevel(Logger::LogLevel::WARN);
        numThreads = atoi(argv[1]);
    }

    const string user = "lutianen";
    const string passwd = "lutianen";
    const string db = "Luxbd";

    EventLoop loop;
    // HttpServer server(&loop, InetAddress(8001), "dummy");
    // server.setHttpCallback(onRequest);
    // server.setThreadNum(numThreads);
    // server.start();
    // loop.loop();

    Application app(&loop, InetAddress(5836), string("Server"),
                    string("/home/lutianen/Lux/app/HTML"), user, passwd, db);
    app.start();
    loop.loop();
}

char favicon[555] = {
    '\x89', 'P',    'N',    'G',    '\xD',  '\xA',  '\x1A', '\xA',  '\x0',
    '\x0',  '\x0',  '\xD',  'I',    'H',    'D',    'R',    '\x0',  '\x0',
    '\x0',  '\x10', '\x0',  '\x0',  '\x0',  '\x10', '\x8',  '\x6',  '\x0',
    '\x0',  '\x0',  '\x1F', '\xF3', '\xFF', 'a',    '\x0',  '\x0',  '\x0',
    '\x19', 't',    'E',    'X',    't',    'S',    'o',    'f',    't',
    'w',    'a',    'r',    'e',    '\x0',  'A',    'd',    'o',    'b',
    'e',    '\x20', 'I',    'm',    'a',    'g',    'e',    'R',    'e',
    'a',    'd',    'y',    'q',    '\xC9', 'e',    '\x3C', '\x0',  '\x0',
    '\x1',  '\xCD', 'I',    'D',    'A',    'T',    'x',    '\xDA', '\x94',
    '\x93', '9',    'H',    '\x3',  'A',    '\x14', '\x86', '\xFF', '\x5D',
    'b',    '\xA7', '\x4',  'R',    '\xC4', 'm',    '\x22', '\x1E', '\xA0',
    'F',    '\x24', '\x8',  '\x16', '\x16', 'v',    '\xA',  '6',    '\xBA',
    'J',    '\x9A', '\x80', '\x8',  'A',    '\xB4', 'q',    '\x85', 'X',
    '\x89', 'G',    '\xB0', 'I',    '\xA9', 'Q',    '\x24', '\xCD', '\xA6',
    '\x8',  '\xA4', 'H',    'c',    '\x91', 'B',    '\xB',  '\xAF', 'V',
    '\xC1', 'F',    '\xB4', '\x15', '\xCF', '\x22', 'X',    '\x98', '\xB',
    'T',    'H',    '\x8A', 'd',    '\x93', '\x8D', '\xFB', 'F',    'g',
    '\xC9', '\x1A', '\x14', '\x7D', '\xF0', 'f',    'v',    'f',    '\xDF',
    '\x7C', '\xEF', '\xE7', 'g',    'F',    '\xA8', '\xD5', 'j',    'H',
    '\x24', '\x12', '\x2A', '\x0',  '\x5',  '\xBF', 'G',    '\xD4', '\xEF',
    '\xF7', '\x2F', '6',    '\xEC', '\x12', '\x20', '\x1E', '\x8F', '\xD7',
    '\xAA', '\xD5', '\xEA', '\xAF', 'I',    '5',    'F',    '\xAA', 'T',
    '\x5F', '\x9F', '\x22', 'A',    '\x2A', '\x95', '\xA',  '\x83', '\xE5',
    'r',    '9',    'd',    '\xB3', 'Y',    '\x96', '\x99', 'L',    '\x6',
    '\xE9', 't',    '\x9A', '\x25', '\x85', '\x2C', '\xCB', 'T',    '\xA7',
    '\xC4', 'b',    '1',    '\xB5', '\x5E', '\x0',  '\x3',  'h',    '\x9A',
    '\xC6', '\x16', '\x82', '\x20', 'X',    'R',    '\x14', 'E',    '6',
    'S',    '\x94', '\xCB', 'e',    'x',    '\xBD', '\x5E', '\xAA', 'U',
    'T',    '\x23', 'L',    '\xC0', '\xE0', '\xE2', '\xC1', '\x8F', '\x0',
    '\x9E', '\xBC', '\x9',  'A',    '\x7C', '\x3E', '\x1F', '\x83', 'D',
    '\x22', '\x11', '\xD5', 'T',    '\x40', '\x3F', '8',    '\x80', 'w',
    '\xE5', '3',    '\x7',  '\xB8', '\x5C', '\x2E', 'H',    '\x92', '\x4',
    '\x87', '\xC3', '\x81', '\x40', '\x20', '\x40', 'g',    '\x98', '\xE9',
    '6',    '\x1A', '\xA6', 'g',    '\x15', '\x4',  '\xE3', '\xD7', '\xC8',
    '\xBD', '\x15', '\xE1', 'i',    '\xB7', 'C',    '\xAB', '\xEA', 'x',
    '\x2F', 'j',    'X',    '\x92', '\xBB', '\x18', '\x20', '\x9F', '\xCF',
    '3',    '\xC3', '\xB8', '\xE9', 'N',    '\xA7', '\xD3', 'l',    'J',
    '\x0',  'i',    '6',    '\x7C', '\x8E', '\xE1', '\xFE', 'V',    '\x84',
    '\xE7', '\x3C', '\x9F', 'r',    '\x2B', '\x3A', 'B',    '\x7B', '7',
    'f',    'w',    '\xAE', '\x8E', '\xE',  '\xF3', '\xBD', 'R',    '\xA9',
    'd',    '\x2',  'B',    '\xAF', '\x85', '2',    'f',    'F',    '\xBA',
    '\xC',  '\xD9', '\x9F', '\x1D', '\x9A', 'l',    '\x22', '\xE6', '\xC7',
    '\x3A', '\x2C', '\x80', '\xEF', '\xC1', '\x15', '\x90', '\x7',  '\x93',
    '\xA2', '\x28', '\xA0', 'S',    'j',    '\xB1', '\xB8', '\xDF', '\x29',
    '5',    'C',    '\xE',  '\x3F', 'X',    '\xFC', '\x98', '\xDA', 'y',
    'j',    'P',    '\x40', '\x0',  '\x87', '\xAE', '\x1B', '\x17', 'B',
    '\xB4', '\x3A', '\x3F', '\xBE', 'y',    '\xC7', '\xA',  '\x26', '\xB6',
    '\xEE', '\xD9', '\x9A', '\x60', '\x14', '\x93', '\xDB', '\x8F', '\xD',
    '\xA',  '\x2E', '\xE9', '\x23', '\x95', '\x29', 'X',    '\x0',  '\x27',
    '\xEB', 'n',    'V',    'p',    '\xBC', '\xD6', '\xCB', '\xD6', 'G',
    '\xAB', '\x3D', 'l',    '\x7D', '\xB8', '\xD2', '\xDD', '\xA0', '\x60',
    '\x83', '\xBA', '\xEF', '\x5F', '\xA4', '\xEA', '\xCC', '\x2',  'N',
    '\xAE', '\x5E', 'p',    '\x1A', '\xEC', '\xB3', '\x40', '9',    '\xAC',
    '\xFE', '\xF2', '\x91', '\x89', 'g',    '\x91', '\x85', '\x21', '\xA8',
    '\x87', '\xB7', 'X',    '\x7E', '\x7E', '\x85', '\xBB', '\xCD', 'N',
    'N',    'b',    't',    '\x40', '\xFA', '\x93', '\x89', '\xEC', '\x1E',
    '\xEC', '\x86', '\x2',  'H',    '\x26', '\x93', '\xD0', 'u',    '\x1D',
    '\x7F', '\x9',  '2',    '\x95', '\xBF', '\x1F', '\xDB', '\xD7', 'c',
    '\x8A', '\x1A', '\xF7', '\x5C', '\xC1', '\xFF', '\x22', 'J',    '\xC3',
    '\x87', '\x0',  '\x3',  '\x0',  'K',    '\xBB', '\xF8', '\xD6', '\x2A',
    'v',    '\x98', 'I',    '\x0',  '\x0',  '\x0',  '\x0',  'I',    'E',
    'N',    'D',    '\xAE', 'B',    '\x60', '\x82',
};
