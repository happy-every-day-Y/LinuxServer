// FILE: ./protocol/protocol/HttpResponse.h
#pragma once
#include "Message.h"
#include <string>
#include <unordered_map>

class HttpResponse : public Message {
public:
    enum class StatusCode {
        OK = 200,
        CREATED = 201,
        NO_CONTENT = 204,
        BAD_REQUEST = 400,
        UNAUTHORIZED = 401,
        FORBIDDEN = 403,
        NOT_FOUND = 404,
        METHOD_NOT_ALLOWED = 405,
        INTERNAL_SERVER_ERROR = 500,
        SERVICE_UNAVAILABLE = 503
    };
    
    HttpResponse() { m_type = MessageType::HTTP_RESPONSE; }
    
    MessageType type() const override { return MessageType::HTTP_RESPONSE; }
    std::string toString() const override;
    
    // 快捷方法
    static std::shared_ptr<HttpResponse> create(StatusCode code = StatusCode::OK,
                                                const std::string& body = "",
                                                const std::string& contentType = "text/plain");
    
    static std::shared_ptr<HttpResponse> createJson(const std::string& json,
                                                    StatusCode code = StatusCode::OK);
    
    static std::shared_ptr<HttpResponse> createHtml(const std::string& html,
                                                   StatusCode code = StatusCode::OK);
    
    // 设置Header
    void setHeader(const std::string& key, const std::string& value) {
        headers[key] = value;
    }
    
    // 设置Cookie
    void setCookie(const std::string& name,
                  const std::string& value,
                  const std::string& path = "/",
                  int maxAge = 86400,
                  bool httpOnly = true);
    
    StatusCode statusCode{StatusCode::OK};
    std::string statusText;
    std::string version{"HTTP/1.1"};
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    
    // 文件路径（用于静态文件服务）
    std::string filePath;
};