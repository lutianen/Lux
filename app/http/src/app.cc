#include <LuxLog/Logger.h>
#include <LuxMySQL/MySQLConn.h>
#include <LuxMySQL/MySQLConnPool.h>
#include <LuxUtils/MTQueue.h>
#include <fcntl.h>
#include <http/app.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <thread>

using namespace Lux;
using namespace Lux::polaris;
using namespace Lux::http;
using namespace Lux::mysql;

bool benchmark = false;

// FIXME use Redis
// username : <mail, password>
std::map<string, std::pair<string, string>> users;

Application::Application(EventLoop* loop, const InetAddress& listenAddr,
                         const string& name, const string& root,
                         const string& dbIpAddr, uint16_t dbPort,
                         const string& dbUser, const string& dbPasswd,
                         const string& dbName)
    : loop_(loop),
      server_(loop, listenAddr, name),
      numThreads_(0),
      realFile_(),
      serverPath_(root),
      fileStat_(),
      fileAddr_(nullptr),
      dbIpAddr_(dbIpAddr),
      dbPort_(dbPort),
      dbUser_(dbUser),
      dbPasswd_(dbPasswd),
      dbName_(dbName),
      connPool_(MySQLConnPool::getInstance()) {
    memZero(realFile_, FILENAME_LEN);

    server_.setHttpCallback(std::bind(&Application::onRequest, this, _1, _2));

    connPool_->init(10, dbIpAddr_, dbPort_, dbUser_, dbPasswd_, dbName_, true,
                    "utf8mb4");

    MYSQL* mysql = nullptr;
    MySQLConn conn(mysql, connPool_);
    conn.execute("SELECT username, mail FROM user;");

    // LOG_INFO << conn.res_.numFields_;
    for (auto& row : conn.stmtRes_.rows_) {
        users[row[0]] = {row[1], row[2]};
    }
}

void Application::onRequest(const HttpRequest& req, HttpResponse* resp) {
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
            for (std::sregex_iterator iter(body.begin(), body.end(), pattern),
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
                MySQLConn conn(mysql, connPool_);
                conn.execute(sqlInsert);

                // 成功
                if (!conn.stmtRes_.rc_) {
                    users[username] = {mail, password};
                    strcpy(realFile_, serverPath_.c_str());
                    int len = strlen(realFile_);
                    const char* index = "/welcome.html";
                    strcat(realFile_, index);
                    delete[] sqlInsert;
                    sqlInsert = nullptr;
                } else {
                    LOG_ERROR << "SELECT error: " << mysql_error(mysql);
                    delete[] sqlInsert;
                    sqlInsert = nullptr;
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
            // 将文件读入到ostringstream对象buf中
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

string readFile2String(const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        LOG_ERROR << "Failed to open file " << filename;
        // exit(EXIT_FAILURE);
    }

    struct stat statbuf;
    if (fstat(fd, &statbuf) < 0) {
        LOG_ERROR << "Failed to get file size";
        // exit(EXIT_FAILURE);
    }

    char* file_contents = static_cast<char*>(
        mmap(0, statbuf.st_size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0));
    if (file_contents == MAP_FAILED) {
        LOG_ERROR << "Failed to mmap file";
        // exit(EXIT_FAILURE);
    }

    string str(file_contents, statbuf.st_size);

    if (munmap(file_contents, statbuf.st_size) < 0) {
        LOG_ERROR << "Failed to munmap file";
        // exit(EXIT_FAILURE);
    }

    close(fd);

    return str;
}

string Application::getHtml() {
    string ret{};

    // NO resource
    if (stat(realFile_, &fileStat_) < 0) return ret;
    // FORBIDDEN REQUEST
    if (!(fileStat_.st_mode & S_IROTH)) return ret;
    // BAD_REQUEST
    if (S_ISDIR(fileStat_.st_mode)) return ret;

    // std::ifstream ifs(realFile_, std::ios::ios_base::binary);
    // if (!ifs.is_open()) {
    //     LOG_ERROR << "Open " << realFile_ << " failed";
    //     return ret;
    // } else {
    //     // 将文件读入到ostringstream对象buf中
    //     std::ostringstream buf{};
    //     char ch;
    //     while (buf && ifs.get(ch)) buf.put(ch);

    //     return buf.str();
    // }

    return readFile2String(realFile_);
}

int main(int argc, char* argv[]) {
    const string dbIp = "192.168.132.131";
    const uint16_t dbPort = 3306;
    const string user = "lutianen";
    const string passwd = "lutianen";
    const string db = "Luxdb";

    if (argc > 1) {
        benchmark = true;

        const string serverName = argv[1];
        const string staticSrcPrefix = argv[2];
        const uint16_t serverPort = atoi(argv[3]);
        int numThreads = atoi(argv[4]);

        EventLoop loop;

        Application app(&loop, InetAddress(5836), serverName, staticSrcPrefix,
                        dbIp, dbPort, user, passwd, db);

        app.setNumThreads(numThreads);
        app.start();
        loop.loop();
    } else {
        printf(
            "Usage: %s serverName staticSrcPrefix "
            "serverPort numThreads \n",
            argv[0]);
    }

    return 0;
}
