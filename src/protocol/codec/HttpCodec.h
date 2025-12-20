#pragma once

#include "Codec.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <sstream>
#include <memory>
#include <vector>
#include <string>

class HttpCodec : public Codec {
public:
    // 构造函数和析构函数
    HttpCodec() = default;
    virtual ~HttpCodec() = default;
    
    // 解码HTTP请求
    std::vector<std::shared_ptr<Message>> decode(Buffer& buf) override;
    
    // 编码HTTP响应或请求
    void encode(const std::shared_ptr<Message>& msg, Buffer& buf) override;
    
    // 快速创建HTTP响应
    static std::string encodeResponse(const std::shared_ptr<HttpResponse>& resp);
    
    // 解析请求行
    static bool parseRequestLine(const std::string& line, 
                                 HttpRequest::Method& method,
                                 std::string& path,
                                 std::string& version);
    
    // 解析头部
    static bool parseHeader(const std::string& line,
                            std::string& key,
                            std::string& value);
    
    // 解析完整的HTTP请求
    static std::shared_ptr<HttpRequest> parseRequest(const std::string& raw);
    
private:
    // 解析HTTP版本
    static std::string parseHttpVersion(const std::string& versionStr);
    
    // 解析HTTP方法
    static HttpRequest::Method parseHttpMethod(const std::string& methodStr);
    
    // 解析查询参数（?key=value&...）
    static void parseQueryParams(const std::string& queryStr,
                                 std::unordered_map<std::string, std::string>& params);
    
    // 解析Cookie头
    static void parseCookies(const std::string& cookieStr,
                             std::unordered_map<std::string, std::string>& cookies);
};