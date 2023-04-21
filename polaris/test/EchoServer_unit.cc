#include <LuxLog/Logger.h>
#include <LuxUtils/Timestamp.h>
#include <bits/stdint-uintn.h>
#include <polaris/polaris.h>
#include <unistd.h>

#include <cstdio>
#include <functional>
#include <utility>

using namespace Lux;
using namespace Lux::polaris;

int numThreads = 0;

class EchoServer {
private:
    EventLoop* loop_;
    TCPServer server_;

    void onConnection(const TCPConnectionPtr& conn) {
        LOG_TRACE << conn->peerAddress().toIpPort() << " -> "
                  << conn->localAddress().toIpPort() << " is "
                  << (conn->connected() ? "up" : "down");

        LOG_INFO << conn->getTcpInfoString();
        conn->send("Hello\n");
    }

    void onMessage(const TCPConnectionPtr& conn, Buffer* buf, Timestamp time) {
        string msg(buf->retrieveAllAsString());
        LOG_TRACE << conn->name() << " recv " << msg.size() << " bytes at "
                  << time.toString();
        if (msg == "exit\n") {
            conn->send("bye\n");
            conn->shutdown();
        }
        if (msg == "quit\n") {
            loop_->quit();
        }
        conn->send(msg);
    }

public:
    EchoServer(EventLoop* loop, const InetAddress& listenAddr)
        : loop_(loop), server_(loop, listenAddr, "polaris EchoServer") {
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, _1));
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, _1, _2, _3));

        server_.setThreadNum(numThreads);
    }

    void start() { server_.start(); }
};

int main(int argc, char* argv[]) {
    Logger::setLogLevel(Logger::LogLevel::INFO);

    LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
    LOG_INFO << "sizeof TCPConnection = " << sizeof(TCPConnection);

    if (argc < 4) {
        LOG_INFO << "EchoServer <ip> <port> <numsThreads>";
    }

    string ip = argv[1];
    uint16_t port = atoi(argv[2]);
    numThreads = atoi(argv[3]);

    EventLoop loop;
    InetAddress listenAddr(ip, port, false);
    EchoServer server(&loop, listenAddr);

    server.start();
    loop.loop();
}
