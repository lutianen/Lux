/**
 * @file HttpResponse.h
 * @brief
 *
 * @author Lux
 */

#pragma once

#include <LuxUtils/Types.h>
#include <polaris/Buffer.h>

#include <map>

namespace Lux {
namespace http {
class HttpResponse {
public:
    enum class HttpStatusCode {
        kUnknown,
        k200Ok = 200,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k404NotFound = 404,
    };

private:
    std::map<string, string> headers_;
    HttpStatusCode statusCode_;
    // FIXME: add http version
    string statusMessage_;
    bool closeConnection_;
    string body_;

public:
    explicit HttpResponse(bool close)
        : statusCode_(HttpStatusCode::kUnknown), closeConnection_(close) {}

    void setStatusCode(HttpStatusCode code) { statusCode_ = code; }

    void setStatusMessage(const string& message) { statusMessage_ = message; }

    void setCloseConnection(bool on) { closeConnection_ = on; }

    bool closeConnection() const { return closeConnection_; }

    void setContentType(const string& contentType) {
        addHeader("Content-Type", contentType);
    }

    // FIXME: replace string with StringPiece
    void addHeader(const string& key, const string& value) {
        headers_[key] = value;
    }

    void setBody(const string& body) { body_ = body; }

    void appendToBuffer(Lux::polaris::Buffer* output) const;
};
}  // namespace http
}  // namespace Lux
