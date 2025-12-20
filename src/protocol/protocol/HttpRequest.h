// FILE: ./protocol/protocol/HttpRequest.h
#pragma once
#include "Message.h"
#include <string>
#include <unordered_map>
#include <vector>

class HttpRequest : public Message {
public:
    enum class Method {
        UNKNOWN,
        GET,
        POST,
        PUT,
        DELETE,
        HEAD,
        OPTIONS,
        PATCH
    };
    
    HttpRequest() { m_type = MessageType::HTTP_REQUEST; }
    
    MessageType type() const override { return MessageType::HTTP_REQUEST; }
    std::string toString() const override;
    
    bool keepAlive() const {
        auto it = headers.find("Connection");
        if (it != headers.end()) {
            return it->second == "keep-alive";
        }
        return version == "HTTP/1.1"; // HTTP/1.1 默认保持连接
    }
    
    // 获取查询参数
    std::string getParam(const std::string& key) const {
        auto it = params.find(key);
        return it != params.end() ? it->second : "";
    }
    
    // 获取Header
    std::string getHeader(const std::string& key) const {
        auto it = headers.find(key);
        return it != headers.end() ? it->second : "";
    }
    
    // 获取Cookie
    std::string getCookie(const std::string& key) const;
    
    // 解析路径参数（如 /user/123 -> {"id": "123"}）
    void parsePathParams(const std::string& pattern);
    
    Method method{Method::GET};
    std::string path;
    std::string version{"HTTP/1.1"};
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> params;
    std::unordered_map<std::string, std::string> pathParams;
    std::string body;
    
    // 解析后的Cookie
    std::unordered_map<std::string, std::string> cookies;
    
    // 客户端地址
    std::string clientIp;
    int clientPort{0};
};