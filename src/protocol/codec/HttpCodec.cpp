#include "HttpCodec.h"
#include "Buffer.h"
#include "Logger.h"
#include <sstream>
#include <algorithm>
#include <cstring>
#include <string>
#include <ctime>
#include <iomanip>
#include <map>

std::vector<std::shared_ptr<Message>> HttpCodec::decode(Buffer& buf) {
    std::vector<std::shared_ptr<Message>> messages;
    
    if (buf.readableBytes() == 0) {
        return messages;
    }
    
    std::string data = buf.retrieveUtf8String();
    if (data.empty()) {
        return messages;
    }
    
    // 查找请求结束位置（\r\n\r\n）
    size_t headerEnd = data.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        // 可能请求不完整，将数据放回缓冲区
        buf.append(data);
        return messages;
    }
    
    size_t pos = 0;
    
    // 解析请求行
    size_t lineEnd = data.find("\r\n", pos);
    if (lineEnd == std::string::npos) {
        buf.append(data);
        return messages;
    }
    
    std::string requestLine = data.substr(pos, lineEnd - pos);
    
    HttpRequest::Method method;
    std::string path, version;
    
    if (!parseRequestLine(requestLine, method, path, version)) {
        LOG_ERROR("Failed to parse request line: {}", requestLine);
        buf.append(data);
        return messages;
    }
    
    auto req = std::make_shared<HttpRequest>();
    req->method = method;
    req->version = version;
    
    // 解析路径和查询参数
    size_t queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        req->path = path.substr(0, queryPos);
        std::string queryStr = path.substr(queryPos + 1);
        parseQueryParams(queryStr, req->params);
    } else {
        req->path = path;
    }
    
    pos = lineEnd + 2; // 跳过 \r\n
    
    // 解析头部
    while (pos < headerEnd) {
        lineEnd = data.find("\r\n", pos);
        if (lineEnd == std::string::npos) {
            break;
        }
        
        std::string headerLine = data.substr(pos, lineEnd - pos);
        if (headerLine.empty()) {
            break;
        }
        
        std::string key, value;
        if (parseHeader(headerLine, key, value)) {
            req->headers[key] = value;
            
            // 解析Cookie
            if (key == "Cookie") {
                parseCookies(value, req->cookies);
            }
        }
        
        pos = lineEnd + 2;
    }
    
    // 解析请求体
    pos = headerEnd + 4; // 跳过 \r\n\r\n
    
    // 检查是否有Content-Length
    if (req->headers.find("Content-Length") != req->headers.end()) {
        try {
            size_t contentLength = std::stoul(req->headers["Content-Length"]);
            if (pos + contentLength <= data.size()) {
                req->body = data.substr(pos, contentLength);
                pos += contentLength;
            } else {
                // 数据不完整，放回缓冲区
                buf.append(data);
                return messages;
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Invalid Content-Length: {}", e.what());
        }
    } else if (req->headers.find("Transfer-Encoding") != req->headers.end() &&
               req->headers["Transfer-Encoding"] == "chunked") {
        // TODO: 实现分块传输编码
        LOG_WARN("Chunked transfer encoding not yet supported");
    }
    
    // 添加请求到消息列表
    messages.push_back(req);
    
    // 检查是否还有更多请求（pipelining）
    if (pos < data.size()) {
        std::string remaining = data.substr(pos);
        buf.append(remaining);
        auto moreMessages = decode(buf);
        messages.insert(messages.end(), moreMessages.begin(), moreMessages.end());
    }
    
    return messages;
}

void HttpCodec::encode(const std::shared_ptr<Message>& msg, Buffer& buf) {
    if (msg->type() == MessageType::HTTP_REQUEST) {
        auto req = std::dynamic_pointer_cast<HttpRequest>(msg);
        if (!req) return;
        
        std::string raw;
        
        // 请求行
        switch (req->method) {
            case HttpRequest::Method::GET: raw += "GET "; break;
            case HttpRequest::Method::POST: raw += "POST "; break;
            case HttpRequest::Method::PUT: raw += "PUT "; break;
            case HttpRequest::Method::DELETE: raw += "DELETE "; break;
            case HttpRequest::Method::HEAD: raw += "HEAD "; break;
            case HttpRequest::Method::OPTIONS: raw += "OPTIONS "; break;
            case HttpRequest::Method::PATCH: raw += "PATCH "; break;
            default: raw += "GET "; break;
        }
        
        // 构建完整路径（包括查询参数）
        std::string fullPath = req->path;
        if (!req->params.empty()) {
            fullPath += "?";
            bool first = true;
            for (const auto& [key, value] : req->params) {
                if (!first) fullPath += "&";
                first = false;
                fullPath += key + "=" + value;
            }
        }
        
        raw += fullPath + " " + req->version + "\r\n";
        
        // 头部
        for (const auto& [key, value] : req->headers) {
            raw += key + ": " + value + "\r\n";
        }
        
        // 如果有Cookie，添加Cookie头
        if (!req->cookies.empty()) {
            std::string cookieHeader;
            bool first = true;
            for (const auto& [key, value] : req->cookies) {
                if (!first) cookieHeader += "; ";
                first = false;
                cookieHeader += key + "=" + value;
            }
            raw += "Cookie: " + cookieHeader + "\r\n";
        }
        
        raw += "\r\n";
        
        // 请求体
        if (!req->body.empty()) {
            raw += req->body;
        }
        
        buf.append(raw);
        
    } else if (msg->type() == MessageType::HTTP_RESPONSE) {
        auto resp = std::dynamic_pointer_cast<HttpResponse>(msg);
        if (!resp) return;
        
        std::string encoded = encodeResponse(resp);
        buf.append(encoded);
    }
}

std::string HttpCodec::encodeResponse(const std::shared_ptr<HttpResponse>& resp) {
    std::ostringstream oss;
    
    // 状态行
    oss << resp->version << " "
        << static_cast<int>(resp->statusCode) << " ";
    
    // 状态文本
    if (!resp->statusText.empty()) {
        oss << resp->statusText;
    } else {
        // 默认状态文本
        static const std::map<HttpResponse::StatusCode, std::string> defaultTexts = {
            {HttpResponse::StatusCode::OK, "OK"},
            {HttpResponse::StatusCode::CREATED, "Created"},
            {HttpResponse::StatusCode::NO_CONTENT, "No Content"},
            {HttpResponse::StatusCode::BAD_REQUEST, "Bad Request"},
            {HttpResponse::StatusCode::UNAUTHORIZED, "Unauthorized"},
            {HttpResponse::StatusCode::FORBIDDEN, "Forbidden"},
            {HttpResponse::StatusCode::NOT_FOUND, "Not Found"},
            {HttpResponse::StatusCode::METHOD_NOT_ALLOWED, "Method Not Allowed"},
            {HttpResponse::StatusCode::INTERNAL_SERVER_ERROR, "Internal Server Error"},
            {HttpResponse::StatusCode::SERVICE_UNAVAILABLE, "Service Unavailable"}
        };
        
        auto it = defaultTexts.find(resp->statusCode);
        if (it != defaultTexts.end()) {
            oss << it->second;
        } else {
            oss << "Unknown";
        }
    }
    oss << "\r\n";
    
    // 自动添加Date头
    time_t now = time(nullptr);
    char dateBuf[64];
    strftime(dateBuf, sizeof(dateBuf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    oss << "Date: " << dateBuf << "\r\n";
    
    // 自动添加Server头
    oss << "Server: ChatServer/1.0\r\n";
    
    // 用户自定义头部
    for (const auto& [key, value] : resp->headers) {
        oss << key << ": " << value << "\r\n";
    }
    
    // 自动添加Content-Length
    if (!resp->body.empty()) {
        oss << "Content-Length: " << resp->body.size() << "\r\n";
    }
    
    // Connection头
    bool keepAlive = true;
    auto connIt = resp->headers.find("Connection");
    if (connIt != resp->headers.end()) {
        keepAlive = (connIt->second == "keep-alive");
    }
    oss << "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n";
    
    oss << "\r\n";
    
    // 响应体
    if (!resp->body.empty()) {
        oss << resp->body;
    }
    
    return oss.str();
}

bool HttpCodec::parseRequestLine(const std::string& line,
                                HttpRequest::Method& method,
                                std::string& path,
                                std::string& version) {
    std::istringstream iss(line);
    std::string methodStr, pathStr, versionStr;
    
    if (!(iss >> methodStr >> pathStr >> versionStr)) {
        return false;
    }
    
    method = parseHttpMethod(methodStr);
    path = pathStr;
    version = parseHttpVersion(versionStr);
    
    return true;
}

bool HttpCodec::parseHeader(const std::string& line,
                           std::string& key,
                           std::string& value) {
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) {
        return false;
    }
    
    key = line.substr(0, colonPos);
    
    // 去除key两端的空格
    size_t keyStart = key.find_first_not_of(" \t");
    size_t keyEnd = key.find_last_not_of(" \t");
    if (keyStart != std::string::npos && keyEnd != std::string::npos) {
        key = key.substr(keyStart, keyEnd - keyStart + 1);
    }
    
    // 获取value
    value = line.substr(colonPos + 1);
    
    // 去除value两端的空格
    size_t valueStart = value.find_first_not_of(" \t");
    size_t valueEnd = value.find_last_not_of(" \t");
    if (valueStart != std::string::npos && valueEnd != std::string::npos) {
        value = value.substr(valueStart, valueEnd - valueStart + 1);
    }
    
    // 如果value以\r结尾，去掉
    if (!value.empty() && value.back() == '\r') {
        value.pop_back();
    }
    
    return true;
}

std::shared_ptr<HttpRequest> HttpCodec::parseRequest(const std::string& raw) {
    auto req = std::make_shared<HttpRequest>();
    
    std::istringstream iss(raw);
    std::string line;
    
    // 解析请求行
    if (!std::getline(iss, line)) {
        return nullptr;
    }
    
    // 去除可能的\r
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    
    HttpRequest::Method method;
    std::string path, version;
    
    if (!parseRequestLine(line, method, path, version)) {
        return nullptr;
    }
    
    req->method = method;
    req->version = version;
    
    // 解析路径和查询参数
    size_t queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        req->path = path.substr(0, queryPos);
        std::string queryStr = path.substr(queryPos + 1);
        parseQueryParams(queryStr, req->params);
    } else {
        req->path = path;
    }
    
    // 解析头部
    while (std::getline(iss, line) && !line.empty()) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (line.empty()) {
            break;
        }
        
        std::string key, value;
        if (parseHeader(line, key, value)) {
            req->headers[key] = value;
            
            // 解析Cookie
            if (key == "Cookie") {
                parseCookies(value, req->cookies);
            }
        }
    }
    
    // 解析请求体
    std::string body((std::istreambuf_iterator<char>(iss)),
                     std::istreambuf_iterator<char>());
    req->body = body;
    
    return req;
}

std::string HttpCodec::parseHttpVersion(const std::string& versionStr) {
    if (versionStr == "HTTP/1.0") {
        return "HTTP/1.0";
    } else if (versionStr == "HTTP/1.1") {
        return "HTTP/1.1";
    } else if (versionStr == "HTTP/2.0") {
        return "HTTP/2.0";
    } else {
        return versionStr;
    }
}

HttpRequest::Method HttpCodec::parseHttpMethod(const std::string& methodStr) {
    if (methodStr == "GET") {
        return HttpRequest::Method::GET;
    } else if (methodStr == "POST") {
        return HttpRequest::Method::POST;
    } else if (methodStr == "PUT") {
        return HttpRequest::Method::PUT;
    } else if (methodStr == "DELETE") {
        return HttpRequest::Method::DELETE;
    } else if (methodStr == "HEAD") {
        return HttpRequest::Method::HEAD;
    } else if (methodStr == "OPTIONS") {
        return HttpRequest::Method::OPTIONS;
    } else if (methodStr == "PATCH") {
        return HttpRequest::Method::PATCH;
    } else {
        return HttpRequest::Method::UNKNOWN;
    }
}

void HttpCodec::parseQueryParams(const std::string& queryStr,
                                std::unordered_map<std::string, std::string>& params) {
    std::istringstream iss(queryStr);
    std::string pair;
    
    while (std::getline(iss, pair, '&')) {
        size_t equalPos = pair.find('=');
        if (equalPos != std::string::npos) {
            std::string key = pair.substr(0, equalPos);
            std::string value = pair.substr(equalPos + 1);
            
            // URL解码（简单实现）
            // TODO: 实现完整的URL解码
            params[key] = value;
        } else {
            params[pair] = "";
        }
    }
}

void HttpCodec::parseCookies(const std::string& cookieStr,
                            std::unordered_map<std::string, std::string>& cookies) {
    std::istringstream iss(cookieStr);
    std::string cookie;
    
    while (std::getline(iss, cookie, ';')) {
        // 去除空格
        size_t start = cookie.find_first_not_of(' ');
        size_t end = cookie.find_last_not_of(' ');
        if (start != std::string::npos && end != std::string::npos) {
            cookie = cookie.substr(start, end - start + 1);
        }
        
        size_t equalPos = cookie.find('=');
        if (equalPos != std::string::npos) {
            std::string key = cookie.substr(0, equalPos);
            std::string value = cookie.substr(equalPos + 1);
            cookies[key] = value;
        }
    }
}