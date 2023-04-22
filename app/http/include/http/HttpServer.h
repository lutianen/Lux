/**
 * @file HttpServer.h
 * @brief
 *
 * @author Lux
 */

#pragma once

#include <polaris/polaris.h>

#include <functional>

#include "polaris/Callbacks.h"

namespace Lux {
namespace http {

class HttpResponse;
class HttpRequest;

/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.
class HttpServer {
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(HttpServer&) = delete;

public:
    using HttpCallback = std::function<void(const HttpRequest&, HttpResponse*)>;

private:
    polaris::TCPServer server_;
    HttpCallback httpCallback_;

public:
    HttpServer(polaris::EventLoop* loop, const polaris::InetAddress& listenAddr,
               const string& name,
               polaris::TCPServer::Option option =
                   polaris::TCPServer::Option::kNoReusePort);

    polaris::EventLoop* getLoop() const { return server_.getLoop(); }

    // Not thread safe, callback be registered before calling start().
    void setHttpCallback(const HttpCallback& cb) { httpCallback_ = cb; }

    void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }

    void start();

private:
    void onConnection(const polaris::TCPConnectionPtr& conn);
    void onMessage(const polaris::TCPConnectionPtr& conn, polaris::Buffer* buf,
                   Timestamp);
    void onWriteCompleteCallback(const polaris::TCPConnectionPtr& conn);
    void onRequest(const polaris::TCPConnectionPtr& conn, const HttpRequest&);
};
}  // namespace http
}  // namespace Lux