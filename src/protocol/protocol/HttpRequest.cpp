// FILE: ./protocol/protocol/HttpRequest.cpp
#include "HttpRequest.h"
#include <sstream>
#include <algorithm>
#include <cstring>

std::string HttpRequest::toString() const {
    std::ostringstream oss;
    
    // 请求行
    switch(method) {
        case Method::GET: oss << "GET "; break;
        case Method::POST: oss << "POST "; break;
        case Method::PUT: oss << "PUT "; break;
        case Method::DELETE: oss << "DELETE "; break;
        case Method::HEAD: oss << "HEAD "; break;
        case Method::OPTIONS: oss << "OPTIONS "; break;
        case Method::PATCH: oss << "PATCH "; break;
        default: oss << "UNKNOWN "; break;
    }
    
    oss << path << " " << version << "\r\n";
    
    // Headers
    for (const auto& [key, value] : headers) {
        oss << key << ": " << value << "\r\n";
    }
    
    oss << "\r\n";
    
    // Body
    if (!body.empty()) {
        oss << body;
    }
    
    return oss.str();
}

std::string HttpRequest::getCookie(const std::string& key) const {
    auto it = cookies.find(key);
    return it != cookies.end() ? it->second : "";
}

void HttpRequest::parsePathParams(const std::string& pattern) {
    // TODO: 实现参数化路径解析
    (void)pattern;  // 消除未使用参数警告
    // 简单实现：/user/{id} -> /user/123
    // 这里需要根据实际情况实现更复杂的匹配
    pathParams.clear();
}