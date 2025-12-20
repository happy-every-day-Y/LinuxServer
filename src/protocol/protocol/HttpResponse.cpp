// FILE: ./protocol/protocol/HttpResponse.cpp
#include "HttpResponse.h"
#include <sstream>
#include <map>
#include <ctime>
#include "Logger.h"

std::string HttpResponse::toString() const {
    std::ostringstream oss;
    
    // 状态行
    oss << version << " " << static_cast<int>(statusCode) << " ";
    
    // 状态文本
    if (!statusText.empty()) {
        oss << statusText;
    } else {
        // 默认状态文本
        static const std::map<StatusCode, std::string> defaultTexts = {
            {StatusCode::OK, "OK"},
            {StatusCode::CREATED, "Created"},
            {StatusCode::NO_CONTENT, "No Content"},
            {StatusCode::BAD_REQUEST, "Bad Request"},
            {StatusCode::UNAUTHORIZED, "Unauthorized"},
            {StatusCode::FORBIDDEN, "Forbidden"},
            {StatusCode::NOT_FOUND, "Not Found"},
            {StatusCode::METHOD_NOT_ALLOWED, "Method Not Allowed"},
            {StatusCode::INTERNAL_SERVER_ERROR, "Internal Server Error"},
            {StatusCode::SERVICE_UNAVAILABLE, "Service Unavailable"}
        };
        
        auto it = defaultTexts.find(statusCode);
        oss << (it != defaultTexts.end() ? it->second : "Unknown");
    }
    
    oss << "\r\n";
    
    // Headers
    for (const auto& [key, value] : headers) {
        oss << key << ": " << value << "\r\n";
    }
    
    // Date头
    time_t now = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    oss << "Date: " << buf << "\r\n";
    
    // Content-Length
    if (!body.empty() || !filePath.empty()) {
        oss << "Content-Length: " << body.size() << "\r\n";
    }
    
    // Connection
    bool keepAlive = headers.find("Connection") != headers.end() && 
                     headers.at("Connection") == "keep-alive";
    oss << "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n";
    
    oss << "\r\n";
    
    // Body
    if (!body.empty()) {
        oss << body;
    }
    
    return oss.str();
}

std::shared_ptr<HttpResponse> HttpResponse::create(StatusCode code, 
                                                   const std::string& body,
                                                   const std::string& contentType) {
    auto resp = std::make_shared<HttpResponse>();
    resp->statusCode = code;
    resp->body = body;
    resp->setHeader("Content-Type", contentType);
    return resp;
}

std::shared_ptr<HttpResponse> HttpResponse::createJson(const std::string& json,
                                                       StatusCode code) {
    auto resp = create(code, json, "application/json; charset=utf-8");
    return resp;
}

std::shared_ptr<HttpResponse> HttpResponse::createHtml(const std::string& html,
                                                       StatusCode code) {
    auto resp = create(code, html, "text/html; charset=utf-8");
    return resp;
}

void HttpResponse::setCookie(const std::string& name,
                             const std::string& value,
                             const std::string& path,
                             int maxAge,
                             bool httpOnly) {
    std::ostringstream oss;
    oss << name << "=" << value << "; Path=" << path << "; Max-Age=" << maxAge;
    if (httpOnly) {
        oss << "; HttpOnly";
    }
    
    // 添加到Header
    auto it = headers.find("Set-Cookie");
    if (it == headers.end()) {
        headers["Set-Cookie"] = oss.str();
    } else {
        // 多个Cookie用换行分隔
        it->second += "\r\nSet-Cookie: " + oss.str();
    }
}